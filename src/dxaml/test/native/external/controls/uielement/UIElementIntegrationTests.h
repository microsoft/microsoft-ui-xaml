// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace UIElement {

    class UIElementIntegrationTests : public WEX::TestClass<UIElementIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(UIElementIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanBringIntoView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a null BringIntoViewOptions")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithOptions)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with an empty TargetRect")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBringIntoviewRequestedEventArgs)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView triggers a BringIntoViewRequested event")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithSmallTargetRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a TargetRect smaller than the target")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithLargeTargetRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a TargetRect larger than the target")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotBringIntoViewWithTargetRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView cannot bring the TargetRect into view because of content boundaries")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithVerticalAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a VerticalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotBringIntoViewWithVerticalAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView cannot use VerticalAlignmentRatio because of content boundaries")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithTargetRectAndVerticalAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a TargetRect and VerticalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithSmallTargetRectAndVerticalAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a small TargetRect and VerticalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithLargeTargetRectAndVerticalAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a large TargetRect and VerticalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoZoomedViewWithHorizontalAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the zoomed-in UIElement into view with a HorizontalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoZoomedViewWithTargetRectAndHorizontalAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the zoomed-in UIElement into view with a TargetRect and HorizontalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with an additional VerticalOffset")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoZoomedViewWithOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the zoomed-in UIElement into view with an additional HorizontalOffset")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithTargetRectAndOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a custom TargetRect and additional VerticalOffset")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewWithAlignmentRatioAndOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the UIElement into view with a VerticalAlignmentRatio and additional VerticalOffset")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotBringIntoViewWithOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView cannot apply the VerticalOffset because of content boundaries")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotBringIntoViewWithAlignmentRatioAndOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView cannot apply the VerticalOffset in addition to VerticalAlignmentRatio because of content boundaries")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInOrthoScrollViewers)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within orthogonal nested ScrollViewers")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInOrthoScrollViewersWithOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within orthogonal nested ScrollViewers with offsets")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInOrthoScrollViewersWithAlignmentRatios)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within orthogonal nested ScrollViewers with HorizontalAlignmentRatio and VerticalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInOrthoScrollViewersWithTargetRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within orthogonal nested ScrollViewers with custom TargetRect")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInOrthoScrollViewersWithTargetRectAndAlignmentRatios)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within orthogonal nested ScrollViewers with custom TargetRect, HorizontalAlignmentRatio and VerticalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInParallelScrollViewers)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within parallel nested ScrollViewers")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInParallelScrollViewersWithOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within parallel nested ScrollViewers with HorizontalOffset")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInParallelScrollViewersWithAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within parallel nested ScrollViewers with HorizontalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInParallelScrollViewersWithAlignmentRatioAndOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within parallel nested ScrollViewers with HorizontalAlignmentRatio and HorizontalOffset")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInParallelScrollViewersWithTargetRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within parallel nested ScrollViewers with custom TargetRect")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringIntoViewInParallelScrollViewersWithTargetRectAndAlignmentRatio)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings the target UIElement into view within parallel nested ScrollViewers with custom TargetRect and HorizontalAlignmentRatio")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringScrollViewerContentIntoView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings a simple ScrollViewer.Content into view")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringTextBoxIntoView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StartBringIntoView brings a TextBox into view")
        END_TEST_METHOD()

    private:
        void BringIntoViewVerticallyWithOptions(
            BringIntoViewOptions^ options,
            Platform::String^ targetName,
            double expectedVerticalOffset,
            Platform::String^ outputSuffix = L"",
            bool startAtMaxOffset = false);
        void BringIntoViewHorizontallyWithOptions(
            BringIntoViewOptions^ options,
            Platform::String^ targetName,
            double expectedHorizontalOffset,
            float zoomFactor = 2.0f,
            Platform::String^ outputSuffix = L"");
        void BringIntoViewInOrthoScrollViewers(
            BringIntoViewOptions^ options,
            Platform::String^ innerScrollViewerName,
            Platform::String^ targetName,
            double expectedInnerHorizontalOffset,
            double expectedOuterVerticalOffset,
            Platform::String^ outputSuffix = L"");
        void BringIntoViewInParallelScrollViewers(
            BringIntoViewOptions^ options,
            Platform::String^ innerScrollViewerName,
            Platform::String^ targetName,
            double expectedInnerHorizontalOffset,
            double expectedOuterHorizontalOffset,
            Platform::String^ outputSuffix = L"",
            bool startInnerAtMaxOffset = false,
            bool startOuterAtMaxOffset = false);
        void BringIntoViewInNestedScrollViewers(
            bool areScrollViewersOrtho,
            BringIntoViewOptions^ options,
            Platform::String^ innerScrollViewerName,
            Platform::String^ targetName,
            double expectedInnerOffset,
            double expectedOuterOffset,
            Platform::String^ outputSuffix = L"",
            bool startInnerAtMaxOffset = false,
            bool startOuterAtMaxOffset = false);
        void BringScrollViewerContentIntoView(
            BringIntoViewOptions^ options,
            double expectedVerticalOffset,
            bool startAtMaxOffset = false);
        void BringTextBoxIntoView(
            BringIntoViewOptions^ options,
            Platform::String^ targetName,
            double expectedInnerVerticalOffset,
            double expectedOuterVerticalOffset,
            bool startInnerAtMaxOffset = false,
            bool startOuterAtMaxOffset = false);
    };

} } } } } }

