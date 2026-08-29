// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "GridIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>

#include <CustomTypeMetadataProvider.h>
#include <Grid.CustomTypes.h>
#include <Grid.CustomTypes.xaml.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Private::Tests::Controls::GridIntegrationTests;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Grid {

    const double GridIntegrationTests::s_rectSize = 30.0;
    const double GridIntegrationTests::s_gridSize = 700.0;
    const double GridIntegrationTests::s_errorMargin = 0.0001;

    bool GridIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool GridIntegrationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
        return true;
    }

    bool GridIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void GridIntegrationTests::CanPerformLayout()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ grid = nullptr;

        // Verify that basic layout is performed correctly.
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  Background='Purple'>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='Auto'/>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='50'/>"
                L"      <RowDefinition Height='50'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='Auto'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='50'/>"
                L"      <ColumnDefinition Width='50'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Border Grid.Row='0' Grid.Column='0'"
                L"      Background='Red' Width='50' Height='50'"
                L"      HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
                L"  <Border Grid.Row='0' Grid.Column='1'"
                L"      Background='Green'"
                L"      HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
                L"  <Border Grid.Row='0' Grid.Column='2'"
                L"      Background='Blue' Width='50' Height='50'"
                L"      HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
                L"  <Border Grid.Row='1' Grid.Column='3' Grid.RowSpan='2'"
                L"      Background='Yellow'"
                L"      HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
                L"  <Border Grid.Row='3' Grid.Column='1' Grid.ColumnSpan='2'"
                L"      Background='Orange'"
                L"      HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        // Verify that changing Grid.Row and Grid.Column triggers a layout
        // pass and that this is performed correctly.
        LOG_OUTPUT(L"Changing layout properties: Grid.Row and Grid.Column.");
        RunOnUIThread([&]()
        {
            xaml_controls::Grid::SetRow(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(0)), 0);
            xaml_controls::Grid::SetColumn(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(0)), 0);
            xaml_controls::Grid::SetRow(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(1)), 1);
            xaml_controls::Grid::SetColumn(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(1)), 0);
            xaml_controls::Grid::SetRow(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(2)), 2);
            xaml_controls::Grid::SetColumn(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(2)), 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

        // Verify that changing Grid.RowSpan and Grid.ColumnSpan triggers a layout
        // pass and that this is performed correctly.
        LOG_OUTPUT(L"Changing layout properties: Grid.RowSpan and Grid.ColumnSpan.");
        RunOnUIThread([&]()
        {
            xaml_controls::Grid::SetRowSpan(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(3)), 1);
            xaml_controls::Grid::SetColumnSpan(safe_cast<xaml::FrameworkElement^>(grid->Children->GetAt(4)), 1);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

        // Verify that the desired size of the panel is correct based on the
        // size of its rows, columns and content.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(grid->DesiredSize.Width, 150.0f);
            VERIFY_ARE_EQUAL(grid->DesiredSize.Height, 150.0f);
        });
    }

    void GridIntegrationTests::VerifyMinWidthAndMinHeight()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ grid = nullptr;

        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='Auto' MinHeight='100'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='Auto' MinWidth='100'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Rectangle Grid.Row='1' Grid.Column='1' Width='50' Height='50' Fill='Red'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(grid->DesiredSize.Width, 100.0f);
            VERIFY_ARE_EQUAL(grid->DesiredSize.Height, 100.0f);
        });
    }

    void GridIntegrationTests::VerifyMaxWidthAndMaxHeight()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ grid = nullptr;

        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='Auto' MaxHeight='50'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='Auto' MaxWidth='50'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Rectangle Grid.Row='1' Grid.Column='1' Width='100' Height='100' Fill='Red'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(grid->DesiredSize.Width, 50.0f);
            VERIFY_ARE_EQUAL(grid->DesiredSize.Height, 50.0f);
        });
    }

    void GridIntegrationTests::VerifyGridLengthForRowsAndColumns()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ grid = nullptr;

        RunOnUIThread([&]()
        {
            // Create a 5x5 Grid to verify that cells using different
            // combinations of GridLengths are sized correctly.
            //
            // ----------------------------------
            // 25px | Auto | Auto | Star | 25px |
            // ----------------------------------
            // Auto |      |      |      |      |
            // ----------------------------------
            // Auto |      |      |      |      |
            // ----------------------------------
            // Star |      |      |      |      |
            // ----------------------------------
            // 25px |      |      |      |      |
            // ----------------------------------

            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  Width='250' Height='250' Background='Red'>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='25'/>"
                L"      <RowDefinition Height='Auto'/>"
                L"      <RowDefinition Height='Auto'/>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='25'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='25'/>"
                L"      <ColumnDefinition Width='Auto'/>"
                L"      <ColumnDefinition Width='Auto'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='25'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Rectangle Grid.Row='1' Grid.Column='1' Width='50' Height='50' Fill='Green'/>"
                L"  <Rectangle Grid.Row='2' Grid.Column='2' Width='75' Height='75' Fill='Blue'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify rows.
            VERIFY_ARE_EQUAL(grid->RowDefinitions->GetAt(0)->ActualHeight, 25.0);
            VERIFY_ARE_EQUAL(grid->RowDefinitions->GetAt(1)->ActualHeight, 50.0);
            VERIFY_ARE_EQUAL(grid->RowDefinitions->GetAt(2)->ActualHeight, 75.0);
            VERIFY_ARE_EQUAL(grid->RowDefinitions->GetAt(3)->ActualHeight, 75.0);
            VERIFY_ARE_EQUAL(grid->RowDefinitions->GetAt(4)->ActualHeight, 25.0);

            // Verify columns.
            VERIFY_ARE_EQUAL(grid->ColumnDefinitions->GetAt(0)->ActualWidth, 25.0);
            VERIFY_ARE_EQUAL(grid->ColumnDefinitions->GetAt(1)->ActualWidth, 50.0);
            VERIFY_ARE_EQUAL(grid->ColumnDefinitions->GetAt(2)->ActualWidth, 75.0);
            VERIFY_ARE_EQUAL(grid->ColumnDefinitions->GetAt(3)->ActualWidth, 75.0);
            VERIFY_ARE_EQUAL(grid->ColumnDefinitions->GetAt(4)->ActualWidth, 25.0);
        });
    }

    void GridIntegrationTests::VerifyPixelTakesPriorityOverStar()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='20'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='20'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            grid->Height = s_rectSize;
            grid->Width = s_rectSize;
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(child->ActualHeight, s_rectSize - 20);
            VERIFY_ARE_EQUAL(child->ActualWidth, s_rectSize - 20);
        });
    }

    void GridIntegrationTests::VerifyAutoTakesPriorityOverStar()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child1 = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='Auto'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='Auto'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Rectangle Grid.Row='1' Grid.Column='1' Fill='Yellow' Height='30' Width='30' />"
                L"</Grid>"));
            grid->Width = s_rectSize*1.5;
            grid->Height = s_rectSize*1.5;
            child1 = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child1);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_LESS_THAN(s_rectSize / 2 - child1->ActualHeight, s_errorMargin);
            VERIFY_IS_LESS_THAN(s_rectSize / 2 - child1->ActualWidth, s_errorMargin);
        });
    }

    void GridIntegrationTests::ThrowsInvalidArgumentForNegativePixelSize()
    {
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT(xaml::GridLengthHelper::FromPixels(-1), Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(xaml::GridLengthHelper::FromPixels(-1), Platform::InvalidArgumentException^);
        });
    }

    void GridIntegrationTests::CanSetCellDimensionsToZero()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='0'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='0'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            grid->SetRow(child, 1);
            grid->SetColumn(child, 1);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(child->ActualHeight, 0);
            VERIFY_ARE_EQUAL(child->ActualWidth, 0);
        });
    }

    void GridIntegrationTests::CanSetCellPixelDimensions()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = ref new xaml_controls::Grid();
            auto row = ref new xaml_controls::RowDefinition();
            auto column = ref new xaml_controls::ColumnDefinition();
            row->Height = xaml::GridLengthHelper::FromPixels(s_rectSize);
            column->Width = xaml::GridLengthHelper::FromPixels(s_rectSize);
            grid->RowDefinitions->Append(row);
            grid->ColumnDefinitions->Append(column);
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_LESS_THAN((s_rectSize - child->ActualHeight), s_errorMargin);
            VERIFY_IS_LESS_THAN((s_rectSize - child->ActualWidth), s_errorMargin);
        });
    }

    void GridIntegrationTests::ValidateLayoutRoundingForPixelDimensions()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = ref new xaml_controls::Grid();
            grid->Width = 400;
            auto column1 = ref new xaml_controls::ColumnDefinition();
            auto column2 = ref new xaml_controls::ColumnDefinition();
            column1->Width = xaml::GridLengthHelper::FromValueAndType(100.25, xaml::GridUnitType::Pixel);
            column2->Width = xaml::GridLengthHelper::FromValueAndType(1, xaml::GridUnitType::Star);
            grid->ColumnDefinitions->Append(column1);
            grid->ColumnDefinitions->Append(column2);
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            grid->SetColumnSpan(child, 2);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto availableSize = xaml_primitives::LayoutInformation::GetAvailableSize(child);
            VERIFY_ARE_EQUAL(availableSize.Width, 400.0f);
        });
    }

    void GridIntegrationTests::CanStarSizedCellsEquallyDivideAllocatedSpace()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_shapes::Rectangle^ child1 = nullptr;
        xaml_shapes::Rectangle^ child2 = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='*'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            child1 = ref new xaml_shapes::Rectangle();
            child2 = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child1);
            grid->Children->Append(child2);
            grid->SetRow(child2, 1);
            grid->SetColumn(child2, 1);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(child1->ActualHeight, child2->ActualHeight);
            VERIFY_ARE_EQUAL(child1->ActualWidth, child2->ActualWidth);
        });
    }

    void GridIntegrationTests::CanNegativelyWeightedCellsThrowInvalidArgument()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT(xaml::GridLengthHelper::FromValueAndType(-1, xaml::GridUnitType::Star), Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(xaml::GridLengthHelper::FromValueAndType(-1, xaml::GridUnitType::Star), Platform::InvalidArgumentException^);
        });
    }

    void GridIntegrationTests::CanZeroWeightedCellsShrinkToZeroSize()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='0*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='0*'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(child->ActualHeight, 0);
            VERIFY_ARE_EQUAL(child->ActualWidth, 0);
        });
    }

    void GridIntegrationTests::CanDivideAllocatedSpacingCorrectlyAccordingToStarWeightings()
    {
        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ child1 = nullptr;
        xaml_shapes::Rectangle^ child2 = nullptr;
        xaml_shapes::Rectangle^ child3 = nullptr;

        const double weighting1 = 0.5;
        const double weighting2 = 1;
        const double weighting3 = 2;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
            L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='0.5*'/>"
                L"      <ColumnDefinition Width='1*'/>"
                L"      <ColumnDefinition Width='2*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='0.5*'/>"
                L"      <RowDefinition Height='1*'/>"
                L"      <RowDefinition Height='2*'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            grid->Height = s_gridSize;
            grid->Width = s_gridSize;
            child1 = ref new xaml_shapes::Rectangle();
            child2 = ref new xaml_shapes::Rectangle();
            child3 = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child1);

            grid->Children->Append(child2);
            grid->SetRow(child2, 1);
            grid->SetColumn(child2, 1);

            grid->Children->Append(child3);
            grid->SetRow(child3, 2);
            grid->SetColumn(child3, 2);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            double totalWeighting = weighting1 + weighting2 + weighting3;

            VERIFY_IS_LESS_THAN((weighting1 * s_gridSize / totalWeighting) - child1->ActualHeight, s_errorMargin);
            VERIFY_IS_LESS_THAN((weighting1 * s_gridSize / totalWeighting) - child1->ActualWidth, s_errorMargin);

            VERIFY_IS_LESS_THAN((weighting2 * s_gridSize / totalWeighting) - child2->ActualHeight, s_errorMargin);
            VERIFY_IS_LESS_THAN((weighting2 * s_gridSize / totalWeighting) - child2->ActualWidth, s_errorMargin);

            VERIFY_IS_LESS_THAN((weighting3 * s_gridSize / totalWeighting) - child3->ActualHeight, s_errorMargin);
            VERIFY_IS_LESS_THAN((weighting3 * s_gridSize / totalWeighting) - child3->ActualWidth, s_errorMargin);
        });
    }

    void GridIntegrationTests::CanAutoSizeCellDefaultToZero()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='auto'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='auto'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(child->ActualHeight, 0);
            VERIFY_ARE_EQUAL(child->ActualWidth, 0);
        });
    }

    void GridIntegrationTests::CanLayoutWithColumnSpan()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='Auto'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='Auto'/>"
                L"      <RowDefinition Height='*'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));

            child = ref new xaml_shapes::Rectangle();
            child->Width = 100;
            child->Height = 100;
            grid->Children->Append(child);
            grid->SetColumn(child, 1);
            grid->SetColumnSpan(child, 2);

            scrollViewer = ref new xaml_controls::ScrollViewer();

            auto rect = ref new xaml_shapes::Rectangle();
            auto redBrush = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

            rect->Fill = redBrush;
            rect->Width = 100;
            rect->Height = 8000;
            scrollViewer->Content = rect;
            grid->Children->Append(scrollViewer);
            grid->SetRow(scrollViewer, 1);

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->ActualHeight, scrollViewer->ViewportHeight);
        });
    }

    void GridIntegrationTests::ThrowsInvalidArgumentForNegativeRowAndColumnSpan()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            auto grid = ref new xaml_controls::Grid();
            VERIFY_THROWS_WINRT(grid->SetColumnSpan(rect, -1), Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(grid->SetRowSpan(rect, -1), Platform::InvalidArgumentException^);
        });
    }

    void GridIntegrationTests::ThrowsInvalidArgumentForZeroRowAndColumnSpan()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            auto grid = ref new xaml_controls::Grid();
            VERIFY_THROWS_WINRT(grid->SetColumnSpan(rect, 0), Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(grid->SetRowSpan(rect, 0), Platform::InvalidArgumentException^);
        });
    }

    void GridIntegrationTests::CanSpanRowsAndColumns()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='*'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            grid->Height = s_rectSize;
            grid->Width = s_rectSize;
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            grid->SetColumnSpan(child, 2);
            grid->SetRowSpan(child, 2);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(child->ActualHeight, 2*s_rectSize/3);
            VERIFY_ARE_EQUAL(child->ActualWidth, 2*s_rectSize/3);
        });
    }

    void GridIntegrationTests::CanSpanAllRowsAndColumns()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='*'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            grid->Height = s_rectSize;
            grid->Width = s_rectSize;
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            grid->SetColumnSpan(child, 3);
            grid->SetRowSpan(child, 3);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_LESS_THAN(s_rectSize - child->ActualHeight, s_errorMargin);
            VERIFY_IS_LESS_THAN(s_rectSize - child->ActualWidth, s_errorMargin);
        });
    }

    void GridIntegrationTests::CanSetSpanToMoreThanTotalRowsAndColumns()
    {
        TestCleanupWrapper cleanup;
        xaml_shapes::Rectangle^ child = nullptr;

        RunOnUIThread([&]()
        {
            auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='*'/>"
                L"      <RowDefinition Height='*'/>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));
            grid->Height = s_rectSize;
            grid->Width = s_rectSize;
            child = ref new xaml_shapes::Rectangle();

            grid->Children->Append(child);
            grid->SetColumnSpan(child, 4);
            grid->SetRowSpan(child, 4);
            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_LESS_THAN(s_rectSize - child->ActualHeight, s_errorMargin);
            VERIFY_IS_LESS_THAN(s_rectSize - child->ActualWidth, s_errorMargin);
        });
    }

    void GridIntegrationTests::ValidateEnsureMinSizeInDefinitionRange()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ grid = nullptr;
        ColumnDefinition^ col0 = nullptr;
        ColumnDefinition^ col1 = nullptr;

        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      Background='Yellow' VerticalAlignment='Center'>"
                L"    <Grid.RowDefinitions>"
                L"        <RowDefinition/>"
                L"        <RowDefinition/>"
                L"    </Grid.RowDefinitions>"
                L"    <Grid.ColumnDefinitions>"
                L"        <ColumnDefinition Width='50'/>"
                L"        <ColumnDefinition Width='50'/>"
                L"    </Grid.ColumnDefinitions>"
                L"    <Border x:Name='b0' Grid.Column='0' Grid.Row='0' Grid.ColumnSpan='2' Background='Red' Opacity='0.5' >"
                L"        <TextBlock TextWrapping='Wrap'>"
                L"          hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world"
                L"          hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world"
                L"        </TextBlock>"
                L"    </Border>"
                L"    <Border x:Name='b1' Grid.Column='0' Grid.Row='1' Background='Blue' Opacity='0.5' >"
                L"        <TextBlock TextWrapping='Wrap'>"
                L"          hello world hello world hello world hello world"
                L"        </TextBlock>"
                L"    </Border>"
                L"    <Border x:Name='b2' Grid.Column='1' Grid.Row='1' Background='Green' Opacity='0.5' >"
                L"        <TextBlock TextWrapping='Wrap'>"
                L"          hello world hello world hello world hello world"
                L"        </TextBlock>"
                L"    </Border>"
                L"</Grid>"));

            auto root = ref new xaml_controls::Grid;
            root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Purple);
            root->Children->Append(grid);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = 10;
            grid->ColumnSpacing = 10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = 0;
            grid->ColumnSpacing = 0;
            grid->ColumnDefinitions->GetAt(0)->Width = GridLengthHelper::FromValueAndType(1, xaml::GridUnitType::Star);
            grid->ColumnDefinitions->GetAt(1)->Width = GridLengthHelper::FromValueAndType(1, xaml::GridUnitType::Star);
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = 10;
            grid->ColumnSpacing = 10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "4");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = 0;
            grid->ColumnSpacing = 0;
            grid->ColumnDefinitions->GetAt(0)->Width = GridLengthHelper::FromValueAndType(1, xaml::GridUnitType::Auto);
            grid->ColumnDefinitions->GetAt(1)->Width = GridLengthHelper::FromValueAndType(50, xaml::GridUnitType::Pixel);
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "5");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = 10;
            grid->ColumnSpacing = 10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "6");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = 0;
            grid->ColumnSpacing = 0;
            Border^ b = (Border^)grid->Children->GetAt(0);
            TextBlock^ tb = (TextBlock^)b->Child;
            tb->Text = "hello world hello world hello world hello world hello world";
            grid->ColumnDefinitions->GetAt(0)->Width = GridLengthHelper::FromValueAndType(1, xaml::GridUnitType::Auto);
            grid->ColumnDefinitions->GetAt(0)->MaxWidth = 50;
            grid->ColumnDefinitions->GetAt(1)->Width = GridLengthHelper::FromValueAndType(50, xaml::GridUnitType::Pixel);
            grid->ColumnDefinitions->GetAt(1)->MaxWidth = 50;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "7");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = 10;
            grid->ColumnSpacing = 10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "8");
    }

    void GridIntegrationTests::BorderChromeForSimpleGrid()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ grid = nullptr;

        // Verify that basic layout is performed correctly.
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  Background='Purple' BorderThickness='20' BorderBrush='Red' CornerRadius='5' Padding='10'>"
                L"  <TextBlock Text='Hello World.' FontSize='20'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        // Verify that changing Grid.BorderThickness and Grid.Padding triggers a layout
        // pass and that this is performed correctly.
        LOG_OUTPUT(L"Changing border properties: Grid.BorderThickness and Grid.Padding.");
        RunOnUIThread([&]()
        {
            grid->BorderThickness = xaml::ThicknessHelper::FromLengths(20, 50, 0, 0);
            grid->Padding = xaml::ThicknessHelper::FromLengths(10, 20, 30, 40);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

        // Verify that changing Grid.BorderBrush will render the new color.
        LOG_OUTPUT(L"Changing border properties: Grid.BorderBrush.");
        RunOnUIThread([&]()
        {
            auto redBrush = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
            grid->BorderBrush = redBrush;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

        // Verify that the desired size of the panel is correct based on the
        // size of its rows, columns and content.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(grid->DesiredSize.Width, 170.0f);
            VERIFY_ARE_EQUAL(grid->DesiredSize.Height, 137.0f);
        });
    }

    void GridIntegrationTests::BorderChromeForComplexGrid()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ grid = nullptr;

        // Verify that basic layout is performed correctly.
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid BorderThickness = '20' BorderBrush='Pink' Background='Purple' Padding='10' CornerRadius='10' x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='100'/>"
                L"      <ColumnDefinition Width='*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='100'/>"
                L"      <RowDefinition Height='*'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Rectangle Width='20' Height='20' Fill='Green'/>"
                L"  <Rectangle Width='20' Height='20' Fill='Red' Grid.Row='2'/>"
                L"  <Rectangle Width='20' Height='20' Fill='Blue' Grid.Column='2'/>"
                L"  <Rectangle Width='20' Height='20' Fill='Yellow' Grid.Row='2' Grid.Column='2'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        // Verify that the desired size of the panel is correct based on the
        // size of its rows, columns and content.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(grid->DesiredSize.Width, 180.0f);
            VERIFY_ARE_EQUAL(grid->DesiredSize.Height, 180.0f);
        });
    }

    void GridIntegrationTests::ValidateSpacing()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ root = nullptr;
        xaml_controls::Grid^ grid = nullptr;

        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      Background='Yellow'"
                L"      RowSpacing='10'"
                L"      HorizontalAlignment='Center'"
                L"      VerticalAlignment='Center'>"
                L"  <Grid.RowDefinitions>"
                L"    <RowDefinition Height='50'/>"
                L"    <RowDefinition Height='Auto'/>"
                L"    <RowDefinition Height='*'/>"
                L"    <RowDefinition Height='50'/>"
                L"    <RowDefinition Height='Auto'/>"
                L"    <RowDefinition Height='*'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Border x:Name='b0' Grid.Row='0' Opacity='0.5'             Width='100' Background='Red' />"
                L"  <Border x:Name='b1' Grid.Row='1' Opacity='0.5' Height='50' Width='100' Background='Blue'/>"
                L"  <Border x:Name='b2' Grid.Row='2' Opacity='0.5' Height='50' Width='100' Background='Green'/>"
                L"  <Border x:Name='b3' Grid.Row='3' Opacity='0.5'             Width='100' Background='Red' />"
                L"  <Border x:Name='b4' Grid.Row='4' Opacity='0.5' Height='50' Width='100' Background='Blue'/>"
                L"  <Border x:Name='b5' Grid.Row='5' Opacity='0.5' Height='50' Width='100' Background='Green'/>"
                L"</Grid>"));

            root = ref new xaml_controls::Grid;
            root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Purple);
            root->Children->Append(grid);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = -10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
    }

    void GridIntegrationTests::ValidateSpacingWithSpans()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ root = nullptr;
        xaml_controls::Grid^ grid = nullptr;

        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      Background='Yellow'"
                L"      RowSpacing='10'"
                L"      ColumnSpacing='10'"
                L"      VerticalAlignment='Center'>"
                L"  <Grid.RowDefinitions>"
                L"    <RowDefinition/>"
                L"    <RowDefinition/>"
                L"    <RowDefinition/>"
                L"  </Grid.RowDefinitions>"
                L"  <Grid.ColumnDefinitions>"
                L"    <ColumnDefinition Width='*'/>"
                L"    <ColumnDefinition Width='*'/>"
                L"    <ColumnDefinition Width='*'/>"
                L"    <ColumnDefinition Width='*'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <Border x:Name='b0' Grid.Row='0' Grid.Column='0' Grid.ColumnSpan='2' Opacity='0.5' Background='Red'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b1' Grid.Row='0' Grid.Column='2' Grid.ColumnSpan='1' Opacity='0.5' Background='Blue'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b2' Grid.Row='0' Grid.Column='3' Grid.ColumnSpan='1' Opacity='0.5' Background='Green'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b3' Grid.Row='1' Grid.Column='0' Grid.ColumnSpan='1' Opacity='0.5' Background='Red'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b4' Grid.Row='1' Grid.Column='1' Grid.ColumnSpan='2' Opacity='0.5' Background='Blue'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b5' Grid.Row='1' Grid.Column='3' Grid.ColumnSpan='1' Opacity='0.5' Background='Green'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b6' Grid.Row='2' Grid.Column='0' Grid.ColumnSpan='1' Opacity='0.5' Background='Red'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b7' Grid.Row='2' Grid.Column='1' Grid.ColumnSpan='1' Opacity='0.5' Background='Blue'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"  <Border x:Name='b8' Grid.Row='2' Grid.Column='2' Grid.ColumnSpan='2' Opacity='0.5' Background='Green'>"
                L"    <TextBlock TextWrapping='Wrap'>"
                L"      hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world"
                L"    </TextBlock>"
                L"  </Border>"
                L"</Grid>"));

            root = ref new xaml_controls::Grid;
            root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Purple);
            root->Children->Append(grid);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        RunOnUIThread([&]()
        {
            grid->RowSpacing = -10;
            grid->ColumnSpacing = -10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
    }

    void GridIntegrationTests::ArrangeOverrideBeforeMeasure()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ grid = nullptr;

        // Verify that basic layout is performed correctly.
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"<local:CustomGrid x:Name='customGrid'>"
                L"  <Rectangle Width='20' Height='20' Fill='Yellow' />"
                L"  <Rectangle Width='20' Height='20' Fill='Yellow' />"
                L"</local:CustomGrid>"
                L"</Grid>"));

            wf::Size size(100, 100);
            LOG_OUTPUT(L"FindName: customGrid");
            auto customGrid = safe_cast<CustomGrid^>(grid->FindName(L"customGrid"));
            customGrid->ArrangeOverride(size);
        });
    }

    void GridIntegrationTests::VerifyRowColumnDefinitionTypeConverter()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ grid = nullptr;

        LOG_OUTPUT(L"Test ColumnDefinition type converter from Pixel-type");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' ColumnDef='50'/>"
                L"</Grid>"));

            auto customTestControl = safe_cast<CustomTestControl^>(grid->FindName(L"customTestControl"));
            ColumnDefinition^ cd = customTestControl->ColumnDef;
            VERIFY_ARE_EQUAL((cd->Width).Value, 50);
            VERIFY_ARE_EQUAL((cd->Width).GridUnitType, xaml::GridUnitType::Pixel);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test RowDefinition type converter from Pixel-type");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' RowDef='50'/>"
                L"</Grid>"));

            auto customTestControl = safe_cast<CustomTestControl^>(grid->FindName(L"customTestControl"));
            RowDefinition^ rd = customTestControl->RowDef;
            VERIFY_ARE_EQUAL((rd->Height).Value, 50);
            VERIFY_ARE_EQUAL((rd->Height).GridUnitType, xaml::GridUnitType::Pixel);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test RowDefinition type converter from Auto-type");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' RowDef='Auto'/>"
                L"</Grid>"));

            auto customTestControl = safe_cast<CustomTestControl^>(grid->FindName(L"customTestControl"));
            RowDefinition^ rd = customTestControl->RowDef;
            VERIFY_ARE_EQUAL((rd->Height).GridUnitType, xaml::GridUnitType::Auto);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test RowDefinition type converter from Star-type");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' RowDef='*'/>"
                L"</Grid>"));

            auto customTestControl = safe_cast<CustomTestControl^>(grid->FindName(L"customTestControl"));
            RowDefinition^ rd = customTestControl->RowDef;
            VERIFY_ARE_EQUAL((rd->Height).Value, 1);
            VERIFY_ARE_EQUAL((rd->Height).GridUnitType, xaml::GridUnitType::Star);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void GridIntegrationTests::ThrowsInvalidArgumentForInvalidRowColumnDefinition() 
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        LOG_OUTPUT(L"ColumnDefinition type converter throws invalid argument with empty string");
        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' ColumnDef=''/>"
                L"</Grid>")), Platform::Exception^);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"RowDefinition type converter throws invalid argument for empty string");
        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' RowDef=''/>"
                L"</Grid>")), Platform::Exception^);
        });

        LOG_OUTPUT(L"RowDefinition type converter throws invalid argument for invalid string");
        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' RowDef='a'/>"
                L"</Grid>")), Platform::Exception^);
        });

        LOG_OUTPUT(L"ColumnDefinition type converter throws invalid argument for invalid string");
        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  xmlns:local='using:Private.Tests.Controls.GridIntegrationTests'>"
                L"  <local:CustomTestControl x:Name='customTestControl' ColumnDef='a'/>"
                L"</Grid>")), Platform::Exception^);
        });
    }

    void GridIntegrationTests::VerifyRowColumnDefinitionContentPropertySyntax() 
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;
        xaml_controls::Grid^ grid = nullptr;

        LOG_OUTPUT(L"Test RowDefinition content property syntax");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.RowDefinitions>"
                L"    <RowDefinition>50</RowDefinition>"
                L"    <RowDefinition>Auto</RowDefinition>"
                L"    <RowDefinition>*</RowDefinition>"
                L"    <RowDefinition></RowDefinition>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));

            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).Value, 50);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).GridUnitType, xaml::GridUnitType::Pixel);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(1)->Height).GridUnitType, xaml::GridUnitType::Auto);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(2)->Height).Value, 1);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(2)->Height).GridUnitType, xaml::GridUnitType::Star);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(3)->Height).Value, 1);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(3)->Height).GridUnitType, xaml::GridUnitType::Star);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test ColumnDefinition content property syntax");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.ColumnDefinitions>"
                L"    <ColumnDefinition>50</ColumnDefinition>"
                L"    <ColumnDefinition>Auto</ColumnDefinition>"
                L"    <ColumnDefinition>*</ColumnDefinition>"
                L"    <ColumnDefinition></ColumnDefinition>"
                L"  </Grid.ColumnDefinitions>"
                L"</Grid>"));

            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).Value, 50);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).GridUnitType, xaml::GridUnitType::Pixel);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(1)->Width).GridUnitType, xaml::GridUnitType::Auto);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(2)->Width).Value, 1);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(2)->Width).GridUnitType, xaml::GridUnitType::Star);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(3)->Width).Value, 1);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(3)->Width).GridUnitType, xaml::GridUnitType::Star);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test that other ColumnDefinition properties can be set with attribute syntax");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.ColumnDefinitions>"
                L"    <ColumnDefinition MaxWidth='400'>50</ColumnDefinition>"
                L"  </Grid.ColumnDefinitions>"
                L"</Grid>"));

            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).Value, 50);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).GridUnitType, xaml::GridUnitType::Pixel);
            VERIFY_ARE_EQUAL(grid->ColumnDefinitions->GetAt(0)->MaxWidth, 400);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test that other RowDefinition properties can be set with attribute syntax");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.RowDefinitions>"
                L"    <RowDefinition MaxHeight='400'>50</RowDefinition>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));

            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).Value, 50);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).GridUnitType, xaml::GridUnitType::Pixel);
            VERIFY_ARE_EQUAL(grid->RowDefinitions->GetAt(0)->MaxHeight, 400);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test that other ColumnDefinition properties can be set with content property syntax"); 
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.ColumnDefinitions>"
                L"    <ColumnDefinition>"
                L"      <ColumnDefinition.MaxWidth>" 
                L"          400" 
                L"      </ColumnDefinition.MaxWidth>"
                L"      50"
                L"    </ColumnDefinition>"
                L"  </Grid.ColumnDefinitions>"
                L"</Grid>"));

            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).Value, 50);
            VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).GridUnitType, xaml::GridUnitType::Pixel);
            VERIFY_ARE_EQUAL(grid->ColumnDefinitions->GetAt(0)->MaxWidth, 400);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test that other RowDefinition properties can be set with content property syntax");
        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.RowDefinitions>"
                L"    <RowDefinition>"
                L"      <RowDefinition.MaxHeight>"
                L"          400"
                L"      </RowDefinition.MaxHeight>"
                L"      50"
                L"    </RowDefinition>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>"));

            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).Value, 50);
            VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).GridUnitType, xaml::GridUnitType::Pixel);
            VERIFY_ARE_EQUAL(grid->RowDefinitions->GetAt(0)->MaxHeight, 400);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Throw invalid argument if ColumnDefinition set with invalid content property"); 
        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.ColumnDefinitions>"
                L"    <ColumnDefinition>a</ColumnDefinition>"
                L"  </Grid.ColumnDefinitions>"
                L"</Grid>")), Platform::Exception^);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Throw invalid argument if RowDefinition set with invalid content property");
        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.RowDefinitions>"
                L"    <RowDefinition>a</RowDefinition>"
                L"  </Grid.RowDefinitions>"
                L"</Grid>")), Platform::Exception^);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Grid
