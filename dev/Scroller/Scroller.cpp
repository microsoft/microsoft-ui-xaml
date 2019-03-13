﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"
#include "InteractionTrackerOwner.h"
#include "Scroller.h"
#include "ScrollOptions.h"
#include "ZoomOptions.h"
#include "ScrollerAutomationPeer.h"
#include "ScrollerTestHooks.h"
#include "ScrollerSnapPoint.h"
#include "Vector.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ScrollerTrace::s_IsDebugOutputEnabled{ false };
bool ScrollerTrace::s_IsVerboseDebugOutputEnabled{ false };

// Number of pixels scrolled when the automation peer requests a line-type change.
const double c_scrollerLineDelta = 16.0;

// Default inertia decay rate used when a IScrollController makes a request for
// an offset change with additional velocity.
const float c_scrollerDefaultInertiaDecayRate = 0.95f;

const winrt::ScrollInfo Scroller::s_noOpScrollInfo{ -1 };
const winrt::ZoomInfo Scroller::s_noOpZoomInfo{ -1 };

Scroller::~Scroller()
{
    SCROLLER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (SharedHelpers::IsRS4OrHigher() &&
        !Scroller::IsInteractionTrackerMouseWheelZoomingEnabled() &&
        m_isListeningToKeystrokes)
    {
        ResetKeyEvents();
    }

    UnhookCompositionTargetRendering();
    UnhookScrollerEvents();
}

Scroller::Scroller()
{
    EnsureProperties();

    SCROLLER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (auto uielementStatics8 = winrt::get_activation_factory<winrt::UIElement, winrt::IUIElementStatics>().try_as<winrt::IUIElementStatics8>())
    {
        uielementStatics8.RegisterAsScrollPort(*this);
    }

    if (!SharedHelpers::IsTH2OrLower())
    {
        HookScrollerEvents();
    }

    // Set the default Transparent background so that hit-testing allows to start a touch manipulation
    // outside the boundaries of the Content, when it's smaller than the Scroller.
    Background(winrt::SolidColorBrush(winrt::Colors::Transparent()));
}

#pragma region Automation Peer Helpers

// Public methods accessed by the ScrollerAutomationPeer class

double Scroller::GetZoomedExtentWidth() const
{
    return m_unzoomedExtentWidth * m_zoomFactor;
}

double Scroller::GetZoomedExtentHeight() const
{
    return m_unzoomedExtentHeight * m_zoomFactor;
}

void Scroller::PageLeft()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset - ViewportWidth());
}

void Scroller::PageRight()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset + ViewportWidth());
}

void Scroller::PageUp()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset - ViewportHeight());
}

void Scroller::PageDown()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset + ViewportHeight());
}

void Scroller::LineLeft()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset - c_scrollerLineDelta);
}

void Scroller::LineRight()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset + c_scrollerLineDelta);
}

void Scroller::LineUp()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset - c_scrollerLineDelta);
}

void Scroller::LineDown()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset + c_scrollerLineDelta);
}

void Scroller::ScrollToHorizontalOffset(double offset)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    ScrollToOffsets(offset /*zoomedHorizontalOffset*/, m_zoomedVerticalOffset /*zoomedVerticalOffset*/);
}

void Scroller::ScrollToVerticalOffset(double offset)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    ScrollToOffsets(m_zoomedHorizontalOffset /*zoomedHorizontalOffset*/, offset /*zoomedVerticalOffset*/);
}

void Scroller::ScrollToOffsets(
    double horizontalOffset, double verticalOffset)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffset, verticalOffset);

    if (m_interactionTracker)
    {
        com_ptr<ScrollOptions> options =
            winrt::make_self<ScrollOptions>(
                winrt::AnimationMode::Disabled,
                winrt::SnapPointsMode::Ignore);

        std::shared_ptr<OffsetsChange> offsetsChange =
            std::make_shared<OffsetsChange>(
                horizontalOffset,
                verticalOffset,
                ScrollerViewKind::Absolute,                
                winrt::IInspectable{ *options }); // NOTE: Using explicit cast to winrt::IInspectable to work around 17532876

        ProcessOffsetsChange(
            InteractionTrackerAsyncOperationTrigger::DirectViewChange,
            offsetsChange,
            -1 /*offsetsChangeId*/,
            false /*isForAsyncOperation*/);
    }
}

#pragma endregion

#pragma region IUIElementOverridesHelper

winrt::AutomationPeer Scroller::OnCreateAutomationPeer()
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    return winrt::make<ScrollerAutomationPeer>(*this);
}

#pragma endregion

#pragma region IScroller

winrt::CompositionPropertySet Scroller::ExpressionAnimationSources()
{
    SetupInteractionTrackerBoundaries();
    EnsureExpressionAnimationSources();

    return m_expressionAnimationSources;
}

double Scroller::HorizontalOffset()
{
    return m_zoomedHorizontalOffset;
}

double Scroller::VerticalOffset()
{
    return m_zoomedVerticalOffset;
}

float Scroller::ZoomFactor()
{
    return m_zoomFactor;
}

double Scroller::ExtentWidth()
{
    return m_unzoomedExtentWidth;
}

double Scroller::ExtentHeight()
{
    return m_unzoomedExtentHeight;
}

double Scroller::ViewportWidth()
{
    return m_viewportWidth;
}

double Scroller::ViewportHeight()
{
    return m_viewportHeight;
}

double Scroller::ScrollableWidth()
{
    return std::max(0.0, GetZoomedExtentWidth() - ViewportWidth());
}

double Scroller::ScrollableHeight()
{
    return std::max(0.0, GetZoomedExtentHeight() - ViewportHeight());
}

winrt::IScrollController Scroller::HorizontalScrollController()
{
    if (m_horizontalScrollController)
    {
        return m_horizontalScrollController.get();
    }

    return nullptr;
}

void Scroller::HorizontalScrollController(winrt::IScrollController const& value)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_PTR, METH_NAME, this, value);

    if (m_horizontalScrollController)
    {
        UnhookHorizontalScrollControllerEvents(m_horizontalScrollController.get());

        if (m_horizontalScrollControllerExpressionAnimationSources)
        {
            MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
            m_horizontalScrollController.get().SetExpressionAnimationSources(
                nullptr /*scrollControllerExpressionAnimationSources*/,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }

    m_horizontalScrollController.set(value);

    if (SharedHelpers::IsRS2OrHigher() && m_interactionTracker)
    {
        SetupScrollControllerVisualInterationSource(ScrollerDimension::HorizontalScroll);
    }

    if (m_horizontalScrollController)
    {
        HookHorizontalScrollControllerEvents(
            m_horizontalScrollController.get(),
            m_horizontalScrollControllerVisualInteractionSource != nullptr /*hasInteractionVisual*/);

        UpdateScrollControllerValues(ScrollerDimension::HorizontalScroll);
        UpdateScrollControllerInteractionsAllowed(ScrollerDimension::HorizontalScroll);

        if (m_horizontalScrollControllerExpressionAnimationSources)
        {
            MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
            m_horizontalScrollController.get().SetExpressionAnimationSources(
                m_horizontalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
}

winrt::IScrollController Scroller::VerticalScrollController()
{
    if (m_verticalScrollController)
    {
        return m_verticalScrollController.get();
    }

    return nullptr;
}

void Scroller::VerticalScrollController(winrt::IScrollController const& value)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_PTR, METH_NAME, this, value);

    if (m_verticalScrollController)
    {
        UnhookVerticalScrollControllerEvents(m_verticalScrollController.get());

        if (m_verticalScrollControllerExpressionAnimationSources)
        {
            MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
            m_verticalScrollController.get().SetExpressionAnimationSources(
                nullptr /*scrollControllerExpressionAnimationSources*/,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }

    m_verticalScrollController.set(value);

    if (SharedHelpers::IsRS2OrHigher() && m_interactionTracker)
    {
        SetupScrollControllerVisualInterationSource(ScrollerDimension::VerticalScroll);
    }

    if (m_verticalScrollController)
    {
        HookVerticalScrollControllerEvents(
            m_verticalScrollController.get(),
            m_verticalScrollControllerVisualInteractionSource != nullptr /*hasInteractionVisual*/);

        UpdateScrollControllerValues(ScrollerDimension::VerticalScroll);
        UpdateScrollControllerInteractionsAllowed(ScrollerDimension::VerticalScroll);

        if (m_verticalScrollControllerExpressionAnimationSources)
        {
            MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
            m_verticalScrollController.get().SetExpressionAnimationSources(
                m_verticalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
}

winrt::InputKind Scroller::IgnoredInputKind()
{
    // Workaround for Bug 17377013: XamlCompiler codegen for Enum CreateFromString always returns boxed int which is wrong for [flags] enums (should be uint)
    // Check if the boxed IgnoredInputKind is an IReference<int> first in which case we unbox as int.
    auto boxedKind = GetValue(s_IgnoredInputKindProperty);
    if (auto boxedInt = boxedKind.try_as<winrt::IReference<int32_t>>())
    {
        return winrt::InputKind{ static_cast<uint32_t>(unbox_value<int32_t>(boxedInt)) };
    }

    return auto_unbox(boxedKind);
}

void Scroller::IgnoredInputKind(winrt::InputKind const& value)
{
    SetValue(s_IgnoredInputKindProperty, box_value(value));
}

winrt::InteractionState Scroller::State()
{
    return m_state;
}

winrt::IVector<winrt::ScrollSnapPointBase> Scroller::HorizontalSnapPoints()
{
    if (!m_horizontalSnapPoints)
    {
        m_horizontalSnapPoints = winrt::make<Vector<winrt::ScrollSnapPointBase>>();

        if (!SharedHelpers::IsTH2OrLower())
        {
            auto horizontalSnapPointsObservableVector = m_horizontalSnapPoints.try_as<winrt::IObservableVector<winrt::ScrollSnapPointBase>>();
            m_horizontalSnapPointsVectorChangedRevoker = horizontalSnapPointsObservableVector.VectorChanged(
                winrt::auto_revoke,
                { this, &Scroller::OnHorizontalSnapPointsVectorChanged });
        }
    }
    return m_horizontalSnapPoints;
}

winrt::IVector<winrt::ScrollSnapPointBase> Scroller::VerticalSnapPoints()
{
    if (!m_verticalSnapPoints)
    {
        m_verticalSnapPoints = winrt::make<Vector<winrt::ScrollSnapPointBase>>();

        if (!SharedHelpers::IsTH2OrLower())
        {
            auto verticalSnapPointsObservableVector = m_verticalSnapPoints.try_as<winrt::IObservableVector<winrt::ScrollSnapPointBase>>();
            m_verticalSnapPointsVectorChangedRevoker = verticalSnapPointsObservableVector.VectorChanged(
                winrt::auto_revoke,
                { this, &Scroller::OnVerticalSnapPointsVectorChanged });
        }
    }
    return m_verticalSnapPoints;
}

winrt::IVector<winrt::ZoomSnapPointBase> Scroller::ZoomSnapPoints()
{
    if (!m_zoomSnapPoints)
    {
        m_zoomSnapPoints = winrt::make<Vector<winrt::ZoomSnapPointBase>>();

        if (!SharedHelpers::IsTH2OrLower())
        {
            auto zoomSnapPointsObservableVector = m_zoomSnapPoints.try_as<winrt::IObservableVector<winrt::ZoomSnapPointBase>>();
            m_zoomSnapPointsVectorChangedRevoker = zoomSnapPointsObservableVector.VectorChanged(
                winrt::auto_revoke,
                { this, &Scroller::OnZoomSnapPointsVectorChanged });
        }
    }
    return m_zoomSnapPoints;
}

winrt::ScrollInfo Scroller::ScrollTo(double horizontalOffset, double verticalOffset)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffset, verticalOffset);

    return ScrollTo(horizontalOffset, verticalOffset, nullptr /*options*/);
}

winrt::ScrollInfo Scroller::ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollOptions const& options)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffset, verticalOffset, TypeLogging::ScrollOptionsToString(options).c_str());

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    int32_t viewChangeId;
    ChangeOffsetsPrivate(
        horizontalOffset /*zoomedHorizontalOffset*/,
        verticalOffset /*zoomedVerticalOffset*/,
        ScrollerViewKind::Absolute,
        options,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        -1 /*existingViewChangeId*/,
        &viewChangeId);

    return winrt::ScrollInfo{ viewChangeId };
}

winrt::ScrollInfo Scroller::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffsetDelta, verticalOffsetDelta);

    return ScrollBy(horizontalOffsetDelta, verticalOffsetDelta, nullptr /*options*/);
}

winrt::ScrollInfo Scroller::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollOptions const& options)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffsetDelta, verticalOffsetDelta, TypeLogging::ScrollOptionsToString(options).c_str());

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    int32_t viewChangeId;
    ChangeOffsetsPrivate(
        horizontalOffsetDelta /*zoomedHorizontalOffset*/,
        verticalOffsetDelta /*zoomedVerticalOffset*/,
        ScrollerViewKind::RelativeToCurrentView,
        options,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        -1 /*existingViewChangeId*/,
        &viewChangeId);

    return winrt::ScrollInfo{ viewChangeId };
}

winrt::ScrollInfo Scroller::ScrollFrom(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(), TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    int32_t viewChangeId;
    ChangeOffsetsWithAdditionalVelocityPrivate(
        offsetsVelocity,
        inertiaDecayRate,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        &viewChangeId);

    return winrt::ScrollInfo{ viewChangeId };
}

winrt::ZoomInfo Scroller::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(), zoomFactor);

    return ZoomTo(zoomFactor, centerPoint, nullptr /*options*/);
}

winrt::ZoomInfo Scroller::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ZoomOptions const& options)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactor);

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    int32_t viewChangeId;
    ChangeZoomFactorPrivate(
        zoomFactor,
        centerPoint,
        ScrollerViewKind::Absolute,
        options,
        &viewChangeId);

    return winrt::ZoomInfo{ viewChangeId };
}

winrt::ZoomInfo Scroller::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        zoomFactorDelta);

    return ZoomBy(zoomFactorDelta, centerPoint, nullptr /*options*/);
}

winrt::ZoomInfo Scroller::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ZoomOptions const& options)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactorDelta);

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    int32_t viewChangeId;
    ChangeZoomFactorPrivate(
        zoomFactorDelta,
        centerPoint,
        ScrollerViewKind::RelativeToCurrentView,
        options,
        &viewChangeId);

    return winrt::ZoomInfo{ viewChangeId };
}

winrt::ZoomInfo Scroller::ZoomFrom(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(),
        zoomFactorVelocity);

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    int32_t viewChangeId;
    ChangeZoomFactorWithAdditionalVelocityPrivate(
        zoomFactorVelocity,
        centerPoint,
        inertiaDecayRate,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        &viewChangeId);

    return winrt::ZoomInfo{ viewChangeId };
}

#pragma endregion

#pragma region IFrameworkElementOverridesHelper

winrt::Size Scroller::MeasureOverride(winrt::Size const& availableSize)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"availableSize:", availableSize.Width, availableSize.Height);

    m_availableSize = availableSize;

    winrt::Size contentDesiredSize{ 0.0f, 0.0f };
    const winrt::UIElement content = Content();

    if (content)
    {
        // The content is measured with infinity in the directions in which it is not constrained, enabling this Scroller
        // to be scrollable in those directions.
        winrt::Size contentAvailableSize
        {
            m_contentOrientation == winrt::ContentOrientation::Vertical ? availableSize.Width : std::numeric_limits<float>::infinity(),
            m_contentOrientation == winrt::ContentOrientation::Horizontal ? availableSize.Height : std::numeric_limits<float>::infinity()
        };

        if (m_contentOrientation != winrt::ContentOrientation::None)
        {
            const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

            if (contentAsFE)
            {
                winrt::Thickness contentMargin = contentAsFE.Margin();

                if (m_contentOrientation == winrt::ContentOrientation::Vertical)
                {
                    // Even though the content's Width is constrained, take into account the MinWidth, Width and MaxWidth values
                    // potentially set on the content so it is allowed to grow accordingly.
                    contentAvailableSize.Width = static_cast<float>(GetComputedMaxWidth(availableSize.Width, contentAsFE));
                }
                else if (m_contentOrientation == winrt::ContentOrientation::Horizontal)
                {
                    // Even though the content's Height is constrained, take into account the MinHeight, Height and MaxHeight values
                    // potentially set on the content so it is allowed to grow accordingly.
                    contentAvailableSize.Height = static_cast<float>(GetComputedMaxHeight(availableSize.Height, contentAsFE));
                }
            }
        }

        content.Measure(contentAvailableSize);
        contentDesiredSize = content.DesiredSize();
    }

    // The framework determines that this Scroller is scrollable when unclippedDesiredSize.Width/Height > desiredSize.Width/Height
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"contentDesiredSize:", contentDesiredSize.Width, contentDesiredSize.Height);

    return contentDesiredSize;
}

winrt::Size Scroller::ArrangeOverride(winrt::Size const& finalSize)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"finalSize", finalSize.Width, finalSize.Height);

    const winrt::UIElement content = Content();
    winrt::Rect finalContentRect{};
    winrt::Size viewport =
    {
        isinf(m_availableSize.Width) ? finalSize.Width : m_availableSize.Width,
        isinf(m_availableSize.Height) ? finalSize.Height : m_availableSize.Height,
    };
    double newUnzoomedExtentWidth = 0.0;
    double newUnzoomedExtentHeight = 0.0;

    if (content)
    {
        float contentLayoutOffsetXDelta = 0.0f;
        float contentLayoutOffsetYDelta = 0.0f;
        bool isAnchoringElementHorizontally = false;
        bool isAnchoringElementVertically = false;
        bool isAnchoringFarEdgeHorizontally = false;
        bool isAnchoringFarEdgeVertically = false;
        winrt::Size contentArrangeSize =
        {
            content.DesiredSize().Width,
            content.DesiredSize().Height
        };

        const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

        if (contentAsFE)
        {
            if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch &&
                isnan(contentAsFE.Width()) &&
                contentArrangeSize.Width < viewport.Width)
            {
                contentArrangeSize.Width = viewport.Width;
            }

            if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch &&
                isnan(contentAsFE.Height()) &&
                contentArrangeSize.Height < viewport.Height)
            {
                contentArrangeSize.Height = viewport.Height;
            }
        }

        finalContentRect =
        {
            m_contentLayoutOffsetX,
            m_contentLayoutOffsetY,
            contentArrangeSize.Width,
            contentArrangeSize.Height
        };

        newUnzoomedExtentWidth = contentArrangeSize.Width;
        newUnzoomedExtentHeight = contentArrangeSize.Height;

        IsAnchoring(&isAnchoringElementHorizontally, &isAnchoringElementVertically, &isAnchoringFarEdgeHorizontally, &isAnchoringFarEdgeVertically);

        MUX_ASSERT(!(isAnchoringElementHorizontally && isAnchoringFarEdgeHorizontally));
        MUX_ASSERT(!(isAnchoringElementVertically && isAnchoringFarEdgeVertically));

        if (isAnchoringElementHorizontally || isAnchoringElementVertically || isAnchoringFarEdgeHorizontally || isAnchoringFarEdgeVertically)
        {
            MUX_ASSERT(m_interactionTracker);

            winrt::Size preArrangeViewportToElementAnchorPointsDistance{ FloatUtil::NaN, FloatUtil::NaN };

            if (isAnchoringElementHorizontally || isAnchoringElementVertically)
            {
                EnsureAnchorElementSelection();
                preArrangeViewportToElementAnchorPointsDistance = ComputeViewportToElementAnchorPointsDistance(
                    m_viewportWidth,
                    m_viewportHeight,
                    true /*isForPreArrange*/);
            }
            else
            {
                ResetAnchorElement();
            }

            SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"content Arrange", TypeLogging::RectToString(finalContentRect).c_str());
            content.Arrange(finalContentRect);

            if (!isnan(preArrangeViewportToElementAnchorPointsDistance.Width) || !isnan(preArrangeViewportToElementAnchorPointsDistance.Height))
            {
                // Using the new viewport sizes to handle the cases where an adjustment needs to be performed because of a Scroller size change.
                winrt::Size postArrangeViewportToElementAnchorPointsDistance = ComputeViewportToElementAnchorPointsDistance(
                    viewport.Width /*viewportWidth*/,
                    viewport.Height /*viewportHeight*/,
                    false /*isForPreArrange*/);

                if (isAnchoringElementHorizontally &&
                    !isnan(preArrangeViewportToElementAnchorPointsDistance.Width) &&
                    !isnan(postArrangeViewportToElementAnchorPointsDistance.Width) &&
                    preArrangeViewportToElementAnchorPointsDistance.Width != postArrangeViewportToElementAnchorPointsDistance.Width)
                {
                    // Perform horizontal offset adjustment due to element anchoring
                    contentLayoutOffsetXDelta = ComputeContentLayoutOffsetDelta(
                        ScrollerDimension::HorizontalScroll,
                        postArrangeViewportToElementAnchorPointsDistance.Width - preArrangeViewportToElementAnchorPointsDistance.Width /*unzoomedDelta*/);
                }

                if (isAnchoringElementVertically &&
                    !isnan(preArrangeViewportToElementAnchorPointsDistance.Height) &&
                    !isnan(postArrangeViewportToElementAnchorPointsDistance.Height) &&
                    preArrangeViewportToElementAnchorPointsDistance.Height != postArrangeViewportToElementAnchorPointsDistance.Height)
                {
                    // Perform vertical offset adjustment due to element anchoring
                    contentLayoutOffsetYDelta = ComputeContentLayoutOffsetDelta(
                        ScrollerDimension::VerticalScroll,
                        postArrangeViewportToElementAnchorPointsDistance.Height - preArrangeViewportToElementAnchorPointsDistance.Height /*unzoomedDelta*/);
                }
            }
        }
        else
        {
            ResetAnchorElement();

            SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"content Arrange", TypeLogging::RectToString(finalContentRect).c_str());
            content.Arrange(finalContentRect);
        }

        SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"content RenderSize", content.RenderSize().Width, content.RenderSize().Height);

        winrt::Thickness contentMargin{};
        double maxUnzoomedExtentWidth = std::numeric_limits<double>::infinity();
        double maxUnzoomedExtentHeight = std::numeric_limits<double>::infinity();

        if (contentAsFE)
        {
            contentMargin = contentAsFE.Margin();

            // Determine the maximum size directly set on the content, if any.
            maxUnzoomedExtentWidth = GetComputedMaxWidth(maxUnzoomedExtentWidth, contentAsFE);
            maxUnzoomedExtentHeight = GetComputedMaxHeight(maxUnzoomedExtentHeight, contentAsFE);
        }

        // Take into account the actual resulting rendering size, in case it's larger than the desired size.
        // But the extent must not exceed the size explicitly set on the content, if any.
        newUnzoomedExtentWidth = std::max(
            newUnzoomedExtentWidth,
            std::max(0.0, content.RenderSize().Width + contentMargin.Left + contentMargin.Right));
        newUnzoomedExtentWidth = std::min(
            newUnzoomedExtentWidth,
            maxUnzoomedExtentWidth);

        newUnzoomedExtentHeight = std::max(
            newUnzoomedExtentHeight,
            std::max(0.0, content.RenderSize().Height + contentMargin.Top + contentMargin.Bottom));
        newUnzoomedExtentHeight = std::min(
            newUnzoomedExtentHeight,
            maxUnzoomedExtentHeight);

        if (isAnchoringFarEdgeHorizontally)
        {
            float unzoomedDelta = 0.0f;

            if (newUnzoomedExtentWidth > m_unzoomedExtentWidth)
            {
                // ExtentWidth grew: Perform horizontal offset adjustment due to edge anchoring
                unzoomedDelta = static_cast<float>(newUnzoomedExtentWidth - m_unzoomedExtentWidth);
            }

            if (static_cast<float>(m_viewportWidth) > viewport.Width)
            {
                // Viewport width shrank: Perform horizontal offset adjustment due to edge anchoring
                unzoomedDelta += (static_cast<float>(m_viewportWidth) - viewport.Width) / m_zoomFactor;
            }

            if (unzoomedDelta != 0.0f)
            {
                MUX_ASSERT(contentLayoutOffsetXDelta == 0.0f);
                contentLayoutOffsetXDelta = ComputeContentLayoutOffsetDelta(ScrollerDimension::HorizontalScroll, unzoomedDelta);
            }
        }

        if (isAnchoringFarEdgeVertically)
        {
            float unzoomedDelta = 0.0f;

            if (newUnzoomedExtentHeight > m_unzoomedExtentHeight)
            {
                // ExtentHeight grew: Perform vertical offset adjustment due to edge anchoring
                unzoomedDelta = static_cast<float>(newUnzoomedExtentHeight - m_unzoomedExtentHeight);
            }

            if (static_cast<float>(m_viewportHeight) > viewport.Height)
            {
                // Viewport height shrank: Perform vertical offset adjustment due to edge anchoring
                unzoomedDelta += (static_cast<float>(m_viewportHeight) - viewport.Height) / m_zoomFactor;
            }

            if (unzoomedDelta != 0.0f)
            {
                MUX_ASSERT(contentLayoutOffsetYDelta == 0.0f);
                contentLayoutOffsetYDelta = ComputeContentLayoutOffsetDelta(ScrollerDimension::VerticalScroll, unzoomedDelta);
            }
        }

        if (contentLayoutOffsetXDelta != 0.0f || contentLayoutOffsetYDelta != 0.0f)
        {
            winrt::Rect contentRectWithDelta =
            {
                m_contentLayoutOffsetX + contentLayoutOffsetXDelta,
                m_contentLayoutOffsetY + contentLayoutOffsetYDelta,
                contentArrangeSize.Width,
                contentArrangeSize.Height
            };

            SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"content Arrange", TypeLogging::RectToString(contentRectWithDelta).c_str());
            content.Arrange(contentRectWithDelta);

            if (contentLayoutOffsetXDelta != 0.0f)
            {
                m_contentLayoutOffsetX += contentLayoutOffsetXDelta;
                UpdateOffset(ScrollerDimension::HorizontalScroll, m_zoomedHorizontalOffset - contentLayoutOffsetXDelta);
                OnContentLayoutOffsetChanged(ScrollerDimension::HorizontalScroll);
            }

            if (contentLayoutOffsetYDelta != 0.0f)
            {
                m_contentLayoutOffsetY += contentLayoutOffsetYDelta;
                UpdateOffset(ScrollerDimension::VerticalScroll, m_zoomedVerticalOffset - contentLayoutOffsetYDelta);
                OnContentLayoutOffsetChanged(ScrollerDimension::VerticalScroll);
            }

            OnViewChanged(contentLayoutOffsetXDelta != 0.0f /*horizontalOffsetChanged*/, contentLayoutOffsetYDelta != 0.0f /*verticalOffsetChanged*/);
        }
    }

    // Set a rectangular clip on this Scroller the same size as the arrange
    // rectangle so the content does not render beyond it.
    winrt::RectangleGeometry rectangleGeometry = safe_cast<winrt::RectangleGeometry>(Clip());

    if (!rectangleGeometry)
    {
        // Ensure that this Scroller has a rectangular clip.
        winrt::RectangleGeometry newRectangleGeometry;
        newRectangleGeometry.Rect();
        Clip(newRectangleGeometry);

        rectangleGeometry = newRectangleGeometry;
    }

    winrt::Rect newClipRect{ 0.0f, 0.0f, viewport.Width, viewport.Height };
    rectangleGeometry.Rect(newClipRect);

    UpdateUnzoomedExtentAndViewport(
        newUnzoomedExtentWidth  /*unzoomedExtentWidth*/,
        newUnzoomedExtentHeight /*unzoomedExtentHeight*/,
        viewport.Width          /*viewportWidth*/,
        viewport.Height         /*viewportHeight*/);

    // We do the following only when effective viewport
    // support is not available. This is to provide downlevel support.
    if (SharedHelpers::IsRS5OrHigher())
    {
        m_isAnchorElementDirty = true;
    }
    else
    {
        ClearAnchorCandidates();
        RaisePostArrange();
    }

    return viewport;
}

#pragma endregion

#pragma region IInteractionTrackerOwner

void Scroller::CustomAnimationStateEntered(
    const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    UpdateState(winrt::InteractionState::Animation);
}

void Scroller::IdleStateEntered(
    const winrt::InteractionTrackerIdleStateEnteredArgs& args)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    UpdateState(winrt::InteractionState::Idle);

    if (!m_interactionTrackerAsyncOperations.empty())
    {
        int32_t requestId = args.RequestId();

        // Complete all operations recorded through ChangeOffsets and ChangeZoomFactor calls.
        if (requestId != 0)
        {
            CompleteInteractionTrackerOperations(
                requestId,
                ScrollerViewChangeResult::Completed   /*operationResult*/,
                ScrollerViewChangeResult::Completed   /*priorNonAnimatedOperationsResult*/,
                ScrollerViewChangeResult::Interrupted /*priorAnimatedOperationsResult*/,
                true  /*completeOperation*/,
                true  /*completePriorNonAnimatedOperations*/,
                true  /*completePriorAnimatedOperations*/);
        }
    }
}

void Scroller::InertiaStateEntered(
    const winrt::InteractionTrackerInertiaStateEnteredArgs& args)
{
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
    // Record the end-of-inertia view for this inertial phase. It may be needed for
    // ChangeOffsets and ChangeZoomFactor calls with RelativeToEndOfInertiaView.

    winrt::float3 naturalRestingPosition = args.NaturalRestingPosition();
    float naturalRestingScale = args.NaturalRestingScale();

    winrt::IReference<winrt::float3> modifiedRestingPosition = args.NaturalRestingPosition();
    winrt::IReference<float> modifiedRestingScale = args.NaturalRestingScale();

    if (modifiedRestingPosition)
    {
        winrt::float3 endOfInertiaPosition = modifiedRestingPosition.Value();
        m_endOfInertiaPosition = { endOfInertiaPosition.x, endOfInertiaPosition.y };
    }
    else
    {
        m_endOfInertiaPosition = { naturalRestingPosition.x, naturalRestingPosition.y };
    }

    if (modifiedRestingScale)
    {
        m_endOfInertiaZoomFactor = modifiedRestingScale.Value();
    }
    else
    {
        m_endOfInertiaZoomFactor = naturalRestingScale;
    }

    SCROLLER_TRACE_INFO(*this, L"%s[0x%p](%d, %s, %f)\n", METH_NAME, this,
        args.RequestId(),
        TypeLogging::Float2ToString(m_endOfInertiaPosition).c_str(),
        m_endOfInertiaZoomFactor);
#endif

    UpdateState(winrt::InteractionState::Inertia);
}

void Scroller::InteractingStateEntered(
    const winrt::InteractionTrackerInteractingStateEnteredArgs& args)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    UpdateState(winrt::InteractionState::Interaction);

    if (!m_interactionTrackerAsyncOperations.empty())
    {
        // Complete all operations recorded through ChangeOffsets and ChangeZoomFactor calls.
        CompleteInteractionTrackerOperations(
            -1 /*requestId*/,
            ScrollerViewChangeResult::Interrupted /*operationResult*/,
            ScrollerViewChangeResult::Completed   /*priorNonAnimatedOperationsResult*/,
            ScrollerViewChangeResult::Interrupted /*priorAnimatedOperationsResult*/,
            true  /*completeOperation*/,
            true  /*completePriorNonAnimatedOperations*/,
            true  /*completePriorAnimatedOperations*/);
    }
}

void Scroller::RequestIgnored(
    const winrt::InteractionTrackerRequestIgnoredArgs& args)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    if (!m_interactionTrackerAsyncOperations.empty())
    {
        // Complete this request alone.
        CompleteInteractionTrackerOperations(
            args.RequestId(),
            ScrollerViewChangeResult::Ignored /*operationResult*/,
            ScrollerViewChangeResult::Ignored /*unused priorNonAnimatedOperationsResult*/,
            ScrollerViewChangeResult::Ignored /*unused priorAnimatedOperationsResult*/,
            true  /*completeOperation*/,
            false /*completePriorNonAnimatedOperations*/,
            false /*completePriorAnimatedOperations*/);
    }
}

void Scroller::ValuesChanged(
    const winrt::InteractionTrackerValuesChangedArgs& args)
{
    bool isScrollerTracingEnabled = IsScrollerTracingEnabled();

    if (isScrollerTracingEnabled || ScrollerTrace::s_IsDebugOutputEnabled || ScrollerTrace::s_IsVerboseDebugOutputEnabled)
    {
        SCROLLER_TRACE_INFO_ENABLED(isScrollerTracingEnabled /*includeTraceLogging*/, *this, L"%s[0x%p](RequestId: %d, View: %f, %f, %f)\n",
            METH_NAME, this, args.RequestId(), args.Position().x, args.Position().y, args.Scale());
    }

    int requestId = args.RequestId();

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationFromRequestId(requestId);

    double oldZoomedHorizontalOffset = m_zoomedHorizontalOffset;
    double oldZoomedVerticalOffset = m_zoomedVerticalOffset;
    float oldZoomFactor = m_zoomFactor;
    winrt::float2 minPosition{};

    m_zoomFactor = args.Scale();

    ComputeMinMaxPositions(m_zoomFactor, &minPosition, nullptr);

    UpdateOffset(ScrollerDimension::HorizontalScroll, args.Position().x - minPosition.x);
    UpdateOffset(ScrollerDimension::VerticalScroll, args.Position().y - minPosition.y);

    if (oldZoomFactor != m_zoomFactor || oldZoomedHorizontalOffset != m_zoomedHorizontalOffset || oldZoomedVerticalOffset != m_zoomedVerticalOffset)
    {
        OnViewChanged(oldZoomedHorizontalOffset != m_zoomedHorizontalOffset /*horizontalOffsetChanged*/,
            oldZoomedVerticalOffset != m_zoomedVerticalOffset /*verticalOffsetChanged*/);
    }

    if (requestId != 0 && !m_interactionTrackerAsyncOperations.empty())
    {
        CompleteInteractionTrackerOperations(
            requestId,
            ScrollerViewChangeResult::Ignored     /*unused operationResult*/,
            ScrollerViewChangeResult::Completed   /*priorNonAnimatedOperationsResult*/,
            ScrollerViewChangeResult::Interrupted /*priorAnimatedOperationsResult*/,
            false /*completeOperation*/,
            true  /*completePriorNonAnimatedOperations*/,
            true  /*completePriorAnimatedOperations*/);
    }
}

#pragma endregion

// Used to perform a flickerless change to the Content's XAML Layout Offset. The InteractionTracker's Position is unaffected, but its Min/MaxPosition expressions
// and the Scroller HorizontalOffset/VerticalOffset property are updated accordingly once the change is incorporated into the XAML layout engine.
float Scroller::ComputeContentLayoutOffsetDelta(ScrollerDimension dimension, float unzoomedDelta) const
{
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);

    float zoomedDelta = unzoomedDelta * m_zoomFactor;

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, unzoomedDelta, m_zoomedHorizontalOffset);

        if (zoomedDelta < 0.0f && -zoomedDelta > m_zoomedHorizontalOffset)
        {
            // Do not let m_zoomedHorizontalOffset step into negative territory.
            zoomedDelta = static_cast<float>(-m_zoomedHorizontalOffset);
        }
        return -zoomedDelta;
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, unzoomedDelta, m_zoomedVerticalOffset);

        if (zoomedDelta < 0.0f && -zoomedDelta > m_zoomedVerticalOffset)
        {
            // Do not let m_zoomedVerticalOffset step into negative territory.
            zoomedDelta = static_cast<float>(-m_zoomedVerticalOffset);
        }
        return -zoomedDelta;
    }
}

float Scroller::ComputeEndOfInertiaZoomFactor() const
{
    if (m_state == winrt::InteractionState::Inertia)
    {
        float endOfInertiaZoomFactor = m_endOfInertiaZoomFactor;

        endOfInertiaZoomFactor = std::max(endOfInertiaZoomFactor, m_interactionTracker.MinScale());
        endOfInertiaZoomFactor = std::min(endOfInertiaZoomFactor, m_interactionTracker.MaxScale());

        return endOfInertiaZoomFactor;
    }
    else
    {
        return m_zoomFactor;
    }
}

winrt::float2 Scroller::ComputeEndOfInertiaPosition()
{
    if (m_state == winrt::InteractionState::Inertia)
    {
        float endOfInertiaZoomFactor = ComputeEndOfInertiaZoomFactor();
        winrt::float2 minPosition{};
        winrt::float2 maxPosition{};
        winrt::float2 endOfInertiaPosition = m_endOfInertiaPosition;

        ComputeMinMaxPositions(endOfInertiaZoomFactor, &minPosition, &maxPosition);

        endOfInertiaPosition = max(endOfInertiaPosition, minPosition);
        endOfInertiaPosition = min(endOfInertiaPosition, maxPosition);

        return endOfInertiaPosition;
    }
    else
    {
        return ComputePositionFromOffsets(m_zoomedHorizontalOffset, m_zoomedVerticalOffset);
    }
}

// Returns zoomed vectors corresponding to InteractionTracker.MinPosition and InteractionTracker.MaxPosition
// Determines the min and max positions of the Scroller.Content based on its size and alignment, and the Scroller size.
void Scroller::ComputeMinMaxPositions(float zoomFactor, _Out_opt_ winrt::float2* minPosition, _Out_opt_ winrt::float2* maxPosition)
{
    MUX_ASSERT(minPosition || maxPosition);

    if (minPosition)
    {
        *minPosition = winrt::float2::zero();
    }

    if (maxPosition)
    {
        *maxPosition = winrt::float2::zero();
    }

    const winrt::UIElement content = Content();

    if (!content)
    {
        return;
    }

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (!contentAsFE)
    {
        return;
    }

    const winrt::Visual scrollerVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    float minPosX = 0.0f;
    float minPosY = 0.0f;
    float maxPosX = 0.0f;
    float maxPosY = 0.0f;
    float extentWidth = static_cast<float>(m_unzoomedExtentWidth);
    float extentHeight = static_cast<float>(m_unzoomedExtentHeight);

    if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
        contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
    {
        float scrollableWidth = extentWidth * zoomFactor - scrollerVisual.Size().x;

        if (minPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, minPosX is scrollableWidth / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, minPosX is 0.
            minPosX = std::min(0.0f, scrollableWidth / 2.0f);
        }

        if (maxPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, maxPosX is scrollableWidth / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, maxPosX is scrollableWidth.
            maxPosX = scrollableWidth;
            if (maxPosX < 0.0f)
            {
                maxPosX /= 2.0f;
            }
        }
    }
    else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
    {
        float scrollableWidth = extentWidth * zoomFactor - scrollerVisual.Size().x;

        if (minPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, minPosX is scrollableWidth so it is right-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, minPosX is 0.
            minPosX = std::min(0.0f, scrollableWidth);
        }

        if (maxPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, maxPosX is -scrollableWidth so it is right-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, maxPosX is scrollableWidth.
            maxPosX = scrollableWidth;
            if (maxPosX < 0.0f)
            {
                maxPosX *= -1.0f;
            }
        }
    }

    if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
        contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
    {
        float scrollableHeight = extentHeight * zoomFactor - scrollerVisual.Size().y;

        if (minPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, minPosY is scrollableHeight / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, minPosY is 0.
            minPosY = std::min(0.0f, scrollableHeight / 2.0f);
        }

        if (maxPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, maxPosY is scrollableHeight / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, maxPosY is scrollableHeight.
            maxPosY = scrollableHeight;
            if (maxPosY < 0.0f)
            {
                maxPosY /= 2.0f;
            }
        }
    }
    else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
    {
        float scrollableHeight = extentHeight * zoomFactor - scrollerVisual.Size().y;

        if (minPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, minPosY is scrollableHeight so it is right-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, minPosY is 0.
            minPosY = std::min(0.0f, scrollableHeight);
        }

        if (maxPosition)
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, maxPosY is -scrollableHeight so it is right-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, maxPosY is scrollableHeight.
            maxPosY = scrollableHeight;
            if (maxPosY < 0.0f)
            {
                maxPosY *= -1.0f;
            }
        }
    }

    if (minPosition)
    {
        *minPosition = winrt::float2(minPosX + m_contentLayoutOffsetX, minPosY + m_contentLayoutOffsetY);
    }

    if (maxPosition)
    {
        *maxPosition = winrt::float2(maxPosX + m_contentLayoutOffsetX, maxPosY + m_contentLayoutOffsetY);
    }
}

// Returns an InteractionTracker Position based on the provided offsets
winrt::float2 Scroller::ComputePositionFromOffsets(double zoomedHorizontalOffset, double zoomedVerticalOffset)
{
    winrt::float2 minPosition{};

    ComputeMinMaxPositions(m_zoomFactor, &minPosition, nullptr);

    return winrt::float2(static_cast<float>(zoomedHorizontalOffset + minPosition.x), static_cast<float>(zoomedVerticalOffset + minPosition.y));
}

// Evaluate what the value will be once the snap points have been applied.
template <typename T>
double Scroller::ComputeValueAfterSnapPoints(
    double value,
    std::set<T, winrtProjectionComparator> const& snapPointsSet)
{
    for (T winrtSnapPoint : snapPointsSet)
    {
        winrt::SnapPointBase winrtSnapPointBase = safe_cast<winrt::SnapPointBase>(winrtSnapPoint);
        auto snapPointBase = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

        if (std::get<0>(snapPointBase->ActualApplicableZone()) <= value &&
            std::get<1>(snapPointBase->ActualApplicableZone()) >= value)
        {
            return snapPointBase->Evaluate(static_cast<float>(value));
        }
    }
    return value;
}

// Returns the zooming center point for mouse-wheel-triggered zooming
winrt::float2 Scroller::ComputeCenterPointerForMouseWheelZooming(const winrt::UIElement& content, const winrt::Point& pointerPosition) const
{
    MUX_ASSERT(!Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());

    float centerPointX = pointerPosition.X;
    float centerPointY = pointerPosition.Y;

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (!contentAsFE)
    {
        return winrt::float2{ centerPointX, centerPointY };
    }

    if (m_unzoomedExtentWidth * m_zoomFactor < m_viewportWidth)
    {
        // Viewport is wider than content
        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            // Use the center of the viewport (and content) as the center point
            centerPointX = static_cast<float>(m_viewportWidth / 2.0);
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            // Use the right viewport edge as the center point
            centerPointX = static_cast<float>(m_viewportWidth);
        }
        else
        {
            MUX_ASSERT(contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Left);

            // Use the left viewport edge as the center point
            centerPointX = 0.0f;
        }
    }
    // else Content is wider than viewport - use the mouse pointer position as the center point.

    if (m_unzoomedExtentHeight * m_zoomFactor < m_viewportHeight)
    {
        // Viewport is wider than content
        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            // Use the center of the viewport (and content) as the center point
            centerPointY = static_cast<float>(m_viewportHeight / 2.0);
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            // Use the bottom viewport edge as the center point
            centerPointY = static_cast<float>(m_viewportHeight);
        }
        else
        {
            MUX_ASSERT(contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Top);

            // Use the top viewport edge as the center point
            centerPointY = 0.0f;
        }
    }
    // else Content is wider than viewport - use the mouse pointer position as the center point.

    return winrt::float2{ centerPointX, centerPointY };
}

void Scroller::ComputeBringIntoViewTargetOffsets(
    const winrt::UIElement& content,
    const winrt::SnapPointsMode& snapPointsMode,
    const winrt::BringIntoViewRequestedEventArgs& requestEventArgs,
    _Out_ double* targetZoomedHorizontalOffset,
    _Out_ double* targetZoomedVerticalOffset,
    _Out_ double* appliedOffsetX,
    _Out_ double* appliedOffsetY,
    _Out_ winrt::Rect* targetRect)
{
    *targetZoomedHorizontalOffset = 0.0;
    *targetZoomedVerticalOffset = 0.0;

    *appliedOffsetX = 0.0;
    *appliedOffsetY = 0.0;

    *targetRect = {};

    const winrt::UIElement target = requestEventArgs.TargetElement();

    MUX_ASSERT(content);
    MUX_ASSERT(target);

    winrt::Rect transformedRect = GetDescendantBounds(content, target, requestEventArgs.TargetRect());

    double targetX = transformedRect.X;
    double targetWidth = transformedRect.Width;
    double targetY = transformedRect.Y;
    double targetHeight = transformedRect.Height;

    if (!isnan(requestEventArgs.HorizontalAlignmentRatio()))
    {
        // Account for the horizontal alignment ratio
        MUX_ASSERT(requestEventArgs.HorizontalAlignmentRatio() >= 0.0 && requestEventArgs.HorizontalAlignmentRatio() <= 1.0);

        targetX += (targetWidth - m_viewportWidth / m_zoomFactor) * requestEventArgs.HorizontalAlignmentRatio();
        targetWidth = m_viewportWidth / m_zoomFactor;
    }

    if (!isnan(requestEventArgs.VerticalAlignmentRatio()))
    {
        // Account for the vertical alignment ratio
        MUX_ASSERT(requestEventArgs.VerticalAlignmentRatio() >= 0.0 && requestEventArgs.VerticalAlignmentRatio() <= 1.0);

        targetY += (targetHeight - m_viewportHeight / m_zoomFactor) * requestEventArgs.VerticalAlignmentRatio();
        targetHeight = m_viewportHeight / m_zoomFactor;
    }

    double targetZoomedHorizontalOffsetTmp = ComputeZoomedOffsetWithMinimalChange(
        m_zoomedHorizontalOffset,
        m_zoomedHorizontalOffset + m_viewportWidth,
        targetX * m_zoomFactor,
        (targetX + targetWidth) * m_zoomFactor);
    double targetZoomedVerticalOffsetTmp = ComputeZoomedOffsetWithMinimalChange(
        m_zoomedVerticalOffset,
        m_zoomedVerticalOffset + m_viewportHeight,
        targetY * m_zoomFactor,
        (targetY + targetHeight) * m_zoomFactor);

    const double scrollableWidth = ScrollableWidth();
    const double scrollableHeight = ScrollableHeight();

    targetZoomedHorizontalOffsetTmp = std::clamp(targetZoomedHorizontalOffsetTmp, 0.0, scrollableWidth);
    targetZoomedVerticalOffsetTmp = std::clamp(targetZoomedVerticalOffsetTmp, 0.0, scrollableHeight);

    const double offsetX = requestEventArgs.HorizontalOffset();
    const double offsetY = requestEventArgs.VerticalOffset();
    double appliedOffsetXTmp = 0.0;
    double appliedOffsetYTmp = 0.0;

    // If the target offset is within bounds and an offset was provided, apply as much of it as possible while remaining within bounds.
    if (offsetX != 0.0 && targetZoomedHorizontalOffsetTmp >= 0.0)
    {
        if (targetZoomedHorizontalOffsetTmp <= scrollableWidth)
        {
            if (offsetX > 0.0)
            {
                appliedOffsetXTmp = std::min(targetZoomedHorizontalOffsetTmp, offsetX);
            }
            else
            {
                appliedOffsetXTmp = -std::min(scrollableWidth - targetZoomedHorizontalOffsetTmp, -offsetX);
            }
            targetZoomedHorizontalOffsetTmp -= appliedOffsetXTmp;
        }
    }

    if (offsetY != 0.0 && targetZoomedVerticalOffsetTmp >= 0.0)
    {
        if (targetZoomedVerticalOffsetTmp <= scrollableHeight)
        {
            if (offsetY > 0.0)
            {
                appliedOffsetYTmp = std::min(targetZoomedVerticalOffsetTmp, offsetY);
            }
            else
            {
                appliedOffsetYTmp = -std::min(scrollableHeight - targetZoomedVerticalOffsetTmp, -offsetY);
            }
            targetZoomedVerticalOffsetTmp -= appliedOffsetYTmp;
        }
    }

    MUX_ASSERT(targetZoomedHorizontalOffsetTmp >= 0.0);
    MUX_ASSERT(targetZoomedVerticalOffsetTmp >= 0.0);
    MUX_ASSERT(targetZoomedHorizontalOffsetTmp <= scrollableWidth);
    MUX_ASSERT(targetZoomedVerticalOffsetTmp <= scrollableHeight);

    if (snapPointsMode == winrt::SnapPointsMode::Default)
    {
        // Finally adjust the target offsets based on snap points
        targetZoomedHorizontalOffsetTmp = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(
            targetZoomedHorizontalOffsetTmp, m_sortedConsolidatedHorizontalSnapPoints);
        targetZoomedVerticalOffsetTmp = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(
            targetZoomedVerticalOffsetTmp, m_sortedConsolidatedVerticalSnapPoints);

        // Make sure the target offsets are within the scrollable boundaries
        targetZoomedHorizontalOffsetTmp = std::clamp(targetZoomedHorizontalOffsetTmp, 0.0, scrollableWidth);
        targetZoomedVerticalOffsetTmp = std::clamp(targetZoomedVerticalOffsetTmp, 0.0, scrollableHeight);

        MUX_ASSERT(targetZoomedHorizontalOffsetTmp >= 0.0);
        MUX_ASSERT(targetZoomedVerticalOffsetTmp >= 0.0);
        MUX_ASSERT(targetZoomedHorizontalOffsetTmp <= scrollableWidth);
        MUX_ASSERT(targetZoomedVerticalOffsetTmp <= scrollableHeight);
    }

    *targetZoomedHorizontalOffset = targetZoomedHorizontalOffsetTmp;
    *targetZoomedVerticalOffset = targetZoomedVerticalOffsetTmp;

    *appliedOffsetX = appliedOffsetXTmp;
    *appliedOffsetY = appliedOffsetYTmp;

    *targetRect = {
        static_cast<float>(targetX),
        static_cast<float>(targetY),
        static_cast<float>(targetWidth),
        static_cast<float>(targetHeight)
    };
}

void Scroller::EnsureExpressionAnimationSources()
{
    if (!m_expressionAnimationSources)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();

        m_expressionAnimationSources = compositor.CreatePropertySet();
        m_expressionAnimationSources.InsertVector2(s_extentSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_viewportSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_offsetSourcePropertyName, { m_contentLayoutOffsetX, m_contentLayoutOffsetY });
        m_expressionAnimationSources.InsertVector2(s_positionSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_minPositionSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_maxPositionSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertScalar(s_zoomFactorSourcePropertyName, 0.0f);

        MUX_ASSERT(m_interactionTracker);
        MUX_ASSERT(!m_positionSourceExpressionAnimation);
        MUX_ASSERT(!m_minPositionSourceExpressionAnimation);
        MUX_ASSERT(!m_maxPositionSourceExpressionAnimation);
        MUX_ASSERT(!m_zoomFactorSourceExpressionAnimation);

        m_positionSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"Vector2(it.Position.X, it.Position.Y)");
        m_positionSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        m_minPositionSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"Vector2(it.MinPosition.X, it.MinPosition.Y)");
        m_minPositionSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        m_maxPositionSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"Vector2(it.MaxPosition.X, it.MaxPosition.Y)");
        m_maxPositionSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        m_zoomFactorSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"it.Scale");
        m_zoomFactorSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        StartExpressionAnimationSourcesAnimations();
        UpdateExpressionAnimationSources();
    }
}

void Scroller::EnsureInteractionTracker()
{
    MUX_ASSERT(!SharedHelpers::IsTH2OrLower());

    if (!m_interactionTracker)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        MUX_ASSERT(!m_interactionTrackerOwner);
        m_interactionTrackerOwner = winrt::make_self<InteractionTrackerOwner>(*this).try_as<winrt::IInteractionTrackerOwner>();

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
        m_interactionTracker = winrt::InteractionTracker::CreateWithOwner(compositor, m_interactionTrackerOwner);
    }
}

void Scroller::EnsureScrollerVisualInteractionSource()
{
    MUX_ASSERT(!SharedHelpers::IsTH2OrLower());

    if (!m_scrollerVisualInteractionSource)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        EnsureInteractionTracker();

        const winrt::Visual scrollerVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);
        winrt::VisualInteractionSource scrollerVisualInteractionSource = winrt::VisualInteractionSource::Create(scrollerVisual);
        m_interactionTracker.InteractionSources().Add(scrollerVisualInteractionSource);
        m_scrollerVisualInteractionSource = scrollerVisualInteractionSource;
        UpdateManipulationRedirectionMode();
        RaiseInteractionSourcesChanged();
    }
}

void Scroller::EnsureScrollControllerVisualInteractionSource(
    const winrt::Visual& interactionVisual,
    ScrollerDimension dimension)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, interactionVisual, dimension);

    MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = winrt::VisualInteractionSource::Create(interactionVisual);
    scrollControllerVisualInteractionSource.ManipulationRedirectionMode(winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadOnly);
    scrollControllerVisualInteractionSource.PositionXChainingMode(winrt::InteractionChainingMode::Never);
    scrollControllerVisualInteractionSource.PositionYChainingMode(winrt::InteractionChainingMode::Never);
    scrollControllerVisualInteractionSource.ScaleChainingMode(winrt::InteractionChainingMode::Never);
    scrollControllerVisualInteractionSource.ScaleSourceMode(winrt::InteractionSourceMode::Disabled);
    m_interactionTracker.InteractionSources().Add(scrollControllerVisualInteractionSource);

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        MUX_ASSERT(!m_horizontalScrollControllerVisualInteractionSource);
        m_horizontalScrollControllerVisualInteractionSource = scrollControllerVisualInteractionSource;
    }
    else
    {
        MUX_ASSERT(!m_verticalScrollControllerVisualInteractionSource);
        m_verticalScrollControllerVisualInteractionSource = scrollControllerVisualInteractionSource;
    }

    RaiseInteractionSourcesChanged();
}

void Scroller::EnsureScrollControllerExpressionAnimationSources(
    ScrollerDimension dimension)
{
    MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
    winrt::CompositionPropertySet scrollControllerExpressionAnimationSources = nullptr;

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        if (m_horizontalScrollControllerExpressionAnimationSources)
        {
            return;
        }

        m_horizontalScrollControllerExpressionAnimationSources = scrollControllerExpressionAnimationSources = compositor.CreatePropertySet();
    }
    else
    {
        if (m_verticalScrollControllerExpressionAnimationSources)
        {
            return;
        }

        m_verticalScrollControllerExpressionAnimationSources = scrollControllerExpressionAnimationSources = compositor.CreatePropertySet();
    }

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, dimension);

    scrollControllerExpressionAnimationSources.InsertScalar(s_minOffsetPropertyName, 0.0f);
    scrollControllerExpressionAnimationSources.InsertScalar(s_maxOffsetPropertyName, 0.0f);
    scrollControllerExpressionAnimationSources.InsertScalar(s_offsetPropertyName, 0.0f);
    scrollControllerExpressionAnimationSources.InsertScalar(s_multiplierPropertyName, 1.0f);

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        MUX_ASSERT(!m_horizontalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(!m_horizontalScrollControllerMaxOffsetExpressionAnimation);

        m_horizontalScrollControllerOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.Position.X - it.MinPosition.X");
        m_horizontalScrollControllerOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_horizontalScrollControllerMaxOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.MaxPosition.X - it.MinPosition.X");
        m_horizontalScrollControllerMaxOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
    }
    else
    {
        MUX_ASSERT(!m_verticalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(!m_verticalScrollControllerMaxOffsetExpressionAnimation);

        m_verticalScrollControllerOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.Position.Y - it.MinPosition.Y");
        m_verticalScrollControllerOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_verticalScrollControllerMaxOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.MaxPosition.Y - it.MinPosition.Y");
        m_verticalScrollControllerMaxOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
    }
}

void Scroller::EnsurePositionBoundariesExpressionAnimations()
{
    MUX_ASSERT(!SharedHelpers::IsTH2OrLower());

    if (!m_minPositionExpressionAnimation || !m_maxPositionExpressionAnimation)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();

        if (!m_minPositionExpressionAnimation)
        {
            m_minPositionExpressionAnimation = compositor.CreateExpressionAnimation();
        }
        if (!m_maxPositionExpressionAnimation)
        {
            m_maxPositionExpressionAnimation = compositor.CreateExpressionAnimation();
        }
    }
}

void Scroller::EnsureTransformExpressionAnimations()
{
    MUX_ASSERT(!SharedHelpers::IsTH2OrLower());

    const bool useTranslationProperty = IsVisualTranslationPropertyAvailable();

    if (((!m_transformMatrixTranslateXExpressionAnimation || !m_transformMatrixTranslateYExpressionAnimation || !m_transformMatrixZoomFactorExpressionAnimation) && !useTranslationProperty) ||
        ((!m_translationExpressionAnimation || !m_zoomFactorExpressionAnimation) && useTranslationProperty))
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();

        if (!m_transformMatrixTranslateXExpressionAnimation && !useTranslationProperty)
        {
            m_transformMatrixTranslateXExpressionAnimation = compositor.CreateExpressionAnimation();
        }

        if (!m_transformMatrixTranslateYExpressionAnimation && !useTranslationProperty)
        {
            m_transformMatrixTranslateYExpressionAnimation = compositor.CreateExpressionAnimation();
        }

        if (!m_translationExpressionAnimation && useTranslationProperty)
        {
            m_translationExpressionAnimation = compositor.CreateExpressionAnimation();
        }

        if (!m_transformMatrixZoomFactorExpressionAnimation && !useTranslationProperty)
        {
            m_transformMatrixZoomFactorExpressionAnimation = compositor.CreateExpressionAnimation();
        }

        if (!m_zoomFactorExpressionAnimation && useTranslationProperty)
        {
            m_zoomFactorExpressionAnimation = compositor.CreateExpressionAnimation();
        }
    }
}

template <typename T>
void Scroller::SetupSnapPoints(
    std::set<T, winrtProjectionComparator>* snapPointsSet,
    ScrollerDimension dimension)
{
    MUX_ASSERT(!SharedHelpers::IsTH2OrLower());
    MUX_ASSERT(snapPointsSet);

    FixSnapPointRanges(snapPointsSet);

    if (!m_interactionTracker)
    {
        EnsureInteractionTracker();
    }
    winrt::Compositor compositor = m_interactionTracker.Compositor();
    winrt::IVector<winrt::InteractionTrackerInertiaModifier> modifiers = winrt::make<Vector<winrt::InteractionTrackerInertiaModifier>>();

    winrt::hstring target = L"";
    winrt::hstring scale = L"";

    switch (dimension)
    {
    case ScrollerDimension::HorizontalZoomFactor:
    case ScrollerDimension::VerticalZoomFactor:
    case ScrollerDimension::Scroll:
        //these ScrollerDimensions are not expected
        MUX_ASSERT(false);
        break;
    case ScrollerDimension::HorizontalScroll:
        target = L"NaturalRestingPosition.x";
        scale = L"this.Target.Scale";
        break;
    case ScrollerDimension::VerticalScroll:
        target = L"NaturalRestingPosition.y";
        scale = L"this.Target.Scale";
        break;
    case ScrollerDimension::ZoomFactor:
        target = L"Scale";
        scale = L"1.0";
        break;
    default:
        MUX_ASSERT(false);
    }

    // For older versions of windows the interaction tracker cannot accept empty collections of inertia modifiers
    if (snapPointsSet->size() == 0)
    {
        winrt::InteractionTrackerInertiaRestingValue modifier = winrt::InteractionTrackerInertiaRestingValue::Create(compositor);
        winrt::ExpressionAnimation conditionExpressionAnimation = compositor.CreateExpressionAnimation(L"false");
        winrt::ExpressionAnimation restingPointExpressionAnimation = compositor.CreateExpressionAnimation(L"this.Target." + target);

        modifier.Condition(conditionExpressionAnimation);
        modifier.RestingValue(restingPointExpressionAnimation);

        modifiers.Append(modifier);
    }
    else
    {
        for (winrt::SnapPointBase snapPoint : *snapPointsSet)
        {
            winrt::InteractionTrackerInertiaRestingValue modifier = GetInertiaRestingValue(
                snapPoint,
                compositor,
                target,
                scale);

            modifiers.Append(modifier);
        }
    }

    switch (dimension)
    {
    case ScrollerDimension::HorizontalZoomFactor:
    case ScrollerDimension::VerticalZoomFactor:
    case ScrollerDimension::Scroll:
        //these ScrollerDimensions are not expected
        MUX_ASSERT(false);
        break;
    case ScrollerDimension::HorizontalScroll:
        m_interactionTracker.ConfigurePositionXInertiaModifiers(modifiers);
        break;
    case ScrollerDimension::VerticalScroll:
        m_interactionTracker.ConfigurePositionYInertiaModifiers(modifiers);
        break;
    case ScrollerDimension::ZoomFactor:
        m_interactionTracker.ConfigureScaleInertiaModifiers(modifiers);
        break;
    default:
        MUX_ASSERT(false);
    }
}

//Snap points which have ApplicableRangeType = Optional are optional snap points, and their ActualApplicableRange should never be expanded beyond their ApplicableRange
//and will only shrink to accommodate other snap points which are positioned such that the midpoint between them is within the specified ApplicableRange.
//Snap points which have ApplicableRangeType = Mandatory are mandatory snap points and their ActualApplicableRange will expand or shrink to ensure that there is no
//space between it and its neighbors. If the neighbors are also mandatory, this point will be the midpoint between them. If the neighbors are optional then this
//point will fall on the midpoint or on the Optional neighbor's edge of ApplicableRange, whichever is furthest. 
template <typename T>
void Scroller::FixSnapPointRanges(
    std::set<T, winrtProjectionComparator>* snapPointsSet)
{
    MUX_ASSERT(snapPointsSet);

    SnapPointBase* currentSnapPoint = nullptr;
    SnapPointBase* previousSnapPoint = nullptr;
    SnapPointBase* nextSnapPoint = nullptr;

    for (T winrtSnapPoint : *snapPointsSet)
    {
        winrt::SnapPointBase winrtSnapPointBase = safe_cast<winrt::SnapPointBase>(winrtSnapPoint);

        previousSnapPoint = currentSnapPoint;
        currentSnapPoint = nextSnapPoint;
        nextSnapPoint = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

        if (currentSnapPoint)
        {
            currentSnapPoint->DetermineActualApplicableZone(previousSnapPoint, nextSnapPoint);
        }
    }
    if (nextSnapPoint)
    {
        nextSnapPoint->DetermineActualApplicableZone(currentSnapPoint, nullptr);
    }
}

void Scroller::SetupInteractionTrackerBoundaries()
{
    if (!m_interactionTracker)
    {
        EnsureInteractionTracker();
        SetupInteractionTrackerZoomFactorBoundaries(
            MinZoomFactor(),
            MaxZoomFactor());
    }

    const winrt::UIElement content = Content();

    if (content && (!m_minPositionExpressionAnimation || !m_maxPositionExpressionAnimation))
    {
        EnsurePositionBoundariesExpressionAnimations();
        SetupPositionBoundariesExpressionAnimations(content);
    }
}

void Scroller::SetupInteractionTrackerZoomFactorBoundaries(
    double minZoomFactor, double maxZoomFactor)
{
    MUX_ASSERT(m_interactionTracker);

#ifdef _DEBUG
    const float oldMinZoomFactorDbg = m_interactionTracker.MinScale();
#endif //_DEBUG
    const float oldMaxZoomFactor = m_interactionTracker.MaxScale();

    minZoomFactor = std::max(0.0, minZoomFactor);
    maxZoomFactor = std::max(minZoomFactor, maxZoomFactor);

    const float newMinZoomFactor = static_cast<float>(minZoomFactor);
    const float newMaxZoomFactor = static_cast<float>(maxZoomFactor);

    if (newMinZoomFactor > oldMaxZoomFactor)
    {
        m_interactionTracker.MaxScale(newMaxZoomFactor);
        m_interactionTracker.MinScale(newMinZoomFactor);
    }
    else
    {
        m_interactionTracker.MinScale(newMinZoomFactor);
        m_interactionTracker.MaxScale(newMaxZoomFactor);
    }
}

// Configures the VisualInteractionSource instance associated with Scroller's Visual.
void Scroller::SetupScrollerVisualInteractionSource()
{
    MUX_ASSERT(m_scrollerVisualInteractionSource);

    SetupVisualInteractionSourceRailingMode(
        m_scrollerVisualInteractionSource,
        ScrollerDimension::HorizontalScroll,
        HorizontalScrollRailingMode());

    SetupVisualInteractionSourceRailingMode(
        m_scrollerVisualInteractionSource,
        ScrollerDimension::VerticalScroll,
        VerticalScrollRailingMode());

    SetupVisualInteractionSourceChainingMode(
        m_scrollerVisualInteractionSource,
        ScrollerDimension::HorizontalScroll,
        HorizontalScrollChainingMode());

    SetupVisualInteractionSourceChainingMode(
        m_scrollerVisualInteractionSource,
        ScrollerDimension::VerticalScroll,
        VerticalScrollChainingMode());

    SetupVisualInteractionSourceChainingMode(
        m_scrollerVisualInteractionSource,
        ScrollerDimension::ZoomFactor,
        ZoomChainingMode());

    UpdateVisualInteractionSourceMode(
        ScrollerDimension::HorizontalScroll);

    UpdateVisualInteractionSourceMode(
        ScrollerDimension::VerticalScroll);

    UpdateKeyEvents();

    SetupVisualInteractionSourceMode(
        m_scrollerVisualInteractionSource,
        ZoomMode());

#ifdef IsMouseWheelZoomDisabled
    if (Scroller::IsInteractionTrackerMouseWheelZoomingEnabled())
    {
        SetupVisualInteractionSourcePointerWheelConfig(
            m_scrollerVisualInteractionSource,
            GetMouseWheelZoomMode());
    }
#endif
}

// Configures the VisualInteractionSource instance associated with the Visual handed in
// through IScrollController::InteractionVisual.
void Scroller::SetupScrollControllerVisualInterationSource(
    ScrollerDimension dimension)
{
    MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = nullptr;
    winrt::Visual interactionVisual = nullptr;

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        scrollControllerVisualInteractionSource = m_horizontalScrollControllerVisualInteractionSource;
        if (m_horizontalScrollController)
        {
            interactionVisual = m_horizontalScrollController.get().InteractionVisual();
        }
    }
    else
    {
        scrollControllerVisualInteractionSource = m_verticalScrollControllerVisualInteractionSource;
        if (m_verticalScrollController)
        {
            interactionVisual = m_verticalScrollController.get().InteractionVisual();
        }
    }

    if (!interactionVisual && scrollControllerVisualInteractionSource)
    {
        // The IScrollController no longer uses a Visual.
        winrt::VisualInteractionSource otherScrollControllerVisualInteractionSource =
            dimension == ScrollerDimension::HorizontalScroll ? m_verticalScrollControllerVisualInteractionSource : m_horizontalScrollControllerVisualInteractionSource;

        if (otherScrollControllerVisualInteractionSource != scrollControllerVisualInteractionSource)
        {
            // The horizontal and vertical IScrollController implementations are not using the same Visual,
            // so the old VisualInteractionSource can be discarded.
            m_interactionTracker.InteractionSources().Remove(scrollControllerVisualInteractionSource);
            StopScrollControllerExpressionAnimationSourcesAnimations(dimension);
            if (dimension == ScrollerDimension::HorizontalScroll)
            {
                m_horizontalScrollControllerVisualInteractionSource = nullptr;
                m_horizontalScrollControllerExpressionAnimationSources = nullptr;
                m_horizontalScrollControllerOffsetExpressionAnimation = nullptr;
                m_horizontalScrollControllerMaxOffsetExpressionAnimation = nullptr;
            }
            else
            {
                m_verticalScrollControllerVisualInteractionSource = nullptr;
                m_verticalScrollControllerExpressionAnimationSources = nullptr;
                m_verticalScrollControllerOffsetExpressionAnimation = nullptr;
                m_verticalScrollControllerMaxOffsetExpressionAnimation = nullptr;
            }

            RaiseInteractionSourcesChanged();
        }
        else
        {
            // The horizontal and vertical IScrollController implementations were using the same Visual,
            // so the old VisualInteractionSource cannot be discarded.
            if (dimension == ScrollerDimension::HorizontalScroll)
            {
                scrollControllerVisualInteractionSource.PositionXSourceMode(winrt::InteractionSourceMode::Disabled);
                scrollControllerVisualInteractionSource.IsPositionXRailsEnabled(false);
            }
            else
            {
                scrollControllerVisualInteractionSource.PositionYSourceMode(winrt::InteractionSourceMode::Disabled);
                scrollControllerVisualInteractionSource.IsPositionYRailsEnabled(false);
            }
        }
        return;
    }
    else if (interactionVisual)
    {
        if (!scrollControllerVisualInteractionSource)
        {
            // The IScrollController now uses a Visual.
            winrt::VisualInteractionSource otherScrollControllerVisualInteractionSource =
                dimension == ScrollerDimension::HorizontalScroll ? m_verticalScrollControllerVisualInteractionSource : m_horizontalScrollControllerVisualInteractionSource;

            if (!otherScrollControllerVisualInteractionSource || otherScrollControllerVisualInteractionSource.Source() != interactionVisual)
            {
                // That Visual is not shared with the other dimension, so create a new VisualInteractionSource for it.
                EnsureScrollControllerVisualInteractionSource(interactionVisual, dimension);
            }
            else
            {
                // That Visual is shared with the other dimension, so share the existing VisualInteractionSource as well.
                if (dimension == ScrollerDimension::HorizontalScroll)
                {
                    m_horizontalScrollControllerVisualInteractionSource = otherScrollControllerVisualInteractionSource;
                }
                else
                {
                    m_verticalScrollControllerVisualInteractionSource = otherScrollControllerVisualInteractionSource;
                }
            }
            EnsureScrollControllerExpressionAnimationSources(dimension);
            StartScrollControllerExpressionAnimationSourcesAnimations(dimension);
        }

        winrt::Orientation orientation;
        bool isRailEnabled;

        // Setup the VisualInteractionSource instance.
        if (dimension == ScrollerDimension::HorizontalScroll)
        {
            orientation = m_horizontalScrollController.get().InteractionVisualScrollOrientation();
            isRailEnabled = m_horizontalScrollController.get().IsInteractionVisualRailEnabled();

            if (orientation == winrt::Orientation::Horizontal)
            {
                m_horizontalScrollControllerVisualInteractionSource.PositionXSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_horizontalScrollControllerVisualInteractionSource.IsPositionXRailsEnabled(isRailEnabled);
            }
            else
            {
                m_horizontalScrollControllerVisualInteractionSource.PositionYSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_horizontalScrollControllerVisualInteractionSource.IsPositionYRailsEnabled(isRailEnabled);
            }
        }
        else
        {
            orientation = m_verticalScrollController.get().InteractionVisualScrollOrientation();
            isRailEnabled = m_verticalScrollController.get().IsInteractionVisualRailEnabled();

            if (orientation == winrt::Orientation::Horizontal)
            {
                m_verticalScrollControllerVisualInteractionSource.PositionXSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_verticalScrollControllerVisualInteractionSource.IsPositionXRailsEnabled(isRailEnabled);
            }
            else
            {
                m_verticalScrollControllerVisualInteractionSource.PositionYSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_verticalScrollControllerVisualInteractionSource.IsPositionYRailsEnabled(isRailEnabled);
            }
        }

        if (!scrollControllerVisualInteractionSource)
        {
            SetupScrollControllerVisualInterationSourcePositionModifiers(
                dimension,
                orientation);
        }
    }
}

// Configures the Position input modifiers of the VisualInteractionSource associated
// with an IScrollController Visual.  The scalar called Multiplier from the CompositionPropertySet
// used in IScrollController::SetExpressionAnimationSources determines the relative speed of
// IScrollController::InteractionVisual compared to the Scroller.Content element.
// The InteractionVisual is clamped based on the Interaction's MinPosition and MaxPosition values.
// Four CompositionConditionalValue instances cover all scenarios:
//  - the Position is moved closer to InteractionTracker.MinPosition while the multiplier is negative.
//  - the Position is moved closer to InteractionTracker.MinPosition while the multiplier is positive.
//  - the Position is moved closer to InteractionTracker.MaxPosition while the multiplier is negative.
//  - the Position is moved closer to InteractionTracker.MaxPosition while the multiplier is positive.
void Scroller::SetupScrollControllerVisualInterationSourcePositionModifiers(
    ScrollerDimension dimension,            // Direction of the Scroller.Content Visual movement.
    const winrt::Orientation& orientation)  // Direction of the IScrollController's Visual movement.
{
    MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = dimension == ScrollerDimension::HorizontalScroll ?
        m_horizontalScrollControllerVisualInteractionSource : m_verticalScrollControllerVisualInteractionSource;
    winrt::CompositionPropertySet scrollControllerExpressionAnimationSources = dimension == ScrollerDimension::HorizontalScroll ?
        m_horizontalScrollControllerExpressionAnimationSources : m_verticalScrollControllerExpressionAnimationSources;

    MUX_ASSERT(scrollControllerVisualInteractionSource);
    MUX_ASSERT(scrollControllerExpressionAnimationSources);

    winrt::Compositor compositor = scrollControllerVisualInteractionSource.Compositor();
    winrt::CompositionConditionalValue ccvs[4]{ nullptr, nullptr, nullptr, nullptr };
    winrt::ExpressionAnimation conditions[4]{ nullptr, nullptr, nullptr, nullptr };
    winrt::ExpressionAnimation values[4]{ nullptr, nullptr, nullptr, nullptr };

    for (int index = 0; index < 4; index++)
    {
        ccvs[index] = winrt::CompositionConditionalValue::Create(compositor);
        conditions[index] = compositor.CreateExpressionAnimation();
        values[index] = compositor.CreateExpressionAnimation();

        ccvs[index].Condition(conditions[index]);
        ccvs[index].Value(values[index]);

        values[index].SetReferenceParameter(L"sceas", scrollControllerExpressionAnimationSources);
        values[index].SetReferenceParameter(L"scvis", scrollControllerVisualInteractionSource);
        values[index].SetReferenceParameter(L"it", m_interactionTracker);
    }

    for (int index = 0; index < 3; index++)
    {
        conditions[index].SetReferenceParameter(L"scvis", scrollControllerVisualInteractionSource);
        conditions[index].SetReferenceParameter(L"sceas", scrollControllerExpressionAnimationSources);
    }
    conditions[3].Expression(L"true");

    auto modifiersVector = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();

    for (int index = 0; index < 4; index++)
    {
        modifiersVector.Append(ccvs[index]);
    }

    if (orientation == winrt::Orientation::Horizontal)
    {
        conditions[0].Expression(L"scvis.DeltaPosition.X < 0.0f && sceas.Multiplier < 0.0f");
        conditions[1].Expression(L"scvis.DeltaPosition.X < 0.0f && sceas.Multiplier >= 0.0f");
        conditions[2].Expression(L"scvis.DeltaPosition.X >= 0.0f && sceas.Multiplier < 0.0f");
        // Case #4 <==> scvis.DeltaPosition.X >= 0.0f && sceas.Multiplier > 0.0f, uses conditions[3].Expression(L"true").
        if (dimension == ScrollerDimension::HorizontalScroll)
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.X - it.MinPosition.X)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.X - it.MaxPosition.X)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionXModifiers(modifiersVector);
        }
        else
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.Y - it.MinPosition.Y)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.Y - it.MaxPosition.Y)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionYModifiers(modifiersVector);

            // When the IScrollController's Visual moves horizontally and controls the vertical Scroller.Content movement, make sure that the
            // vertical finger movements do not affect the Scroller.Content vertically. The vertical component of the finger movement is filtered out.
            winrt::CompositionConditionalValue ccvOrtho = winrt::CompositionConditionalValue::Create(compositor);
            winrt::ExpressionAnimation conditionOrtho = compositor.CreateExpressionAnimation(L"true");
            winrt::ExpressionAnimation valueOrtho = compositor.CreateExpressionAnimation(L"0");
            ccvOrtho.Condition(conditionOrtho);
            ccvOrtho.Value(valueOrtho);

            auto modifiersVectorOrtho = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();
            modifiersVectorOrtho.Append(ccvOrtho);

            scrollControllerVisualInteractionSource.ConfigureDeltaPositionXModifiers(modifiersVectorOrtho);
        }
    }
    else
    {
        conditions[0].Expression(L"scvis.DeltaPosition.Y < 0.0f && sceas.Multiplier < 0.0f");
        conditions[1].Expression(L"scvis.DeltaPosition.Y < 0.0f && sceas.Multiplier >= 0.0f");
        conditions[2].Expression(L"scvis.DeltaPosition.Y >= 0.0f && sceas.Multiplier < 0.0f");
        // Case #4 <==> scvis.DeltaPosition.Y >= 0.0f && sceas.Multiplier > 0.0f, uses conditions[3].Expression(L"true").
        if (dimension == ScrollerDimension::HorizontalScroll)
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.X - it.MinPosition.X)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.X - it.MaxPosition.X)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionXModifiers(modifiersVector);

            // When the IScrollController's Visual moves vertically and controls the horizontal Scroller.Content movement, make sure that the
            // horizontal finger movements do not affect the Scroller.Content horizontally. The horizontal component of the finger movement is filtered out.
            winrt::CompositionConditionalValue ccvOrtho = winrt::CompositionConditionalValue::Create(compositor);
            winrt::ExpressionAnimation conditionOrtho = compositor.CreateExpressionAnimation(L"true");
            winrt::ExpressionAnimation valueOrtho = compositor.CreateExpressionAnimation(L"0");
            ccvOrtho.Condition(conditionOrtho);
            ccvOrtho.Value(valueOrtho);

            auto modifiersVectorOrtho = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();
            modifiersVectorOrtho.Append(ccvOrtho);

            scrollControllerVisualInteractionSource.ConfigureDeltaPositionYModifiers(modifiersVectorOrtho);
        }
        else
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.Y - it.MinPosition.Y)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.Y - it.MaxPosition.Y)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionYModifiers(modifiersVector);
        }
    }
}

void Scroller::SetupVisualInteractionSourceRailingMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollerDimension dimension,
    const winrt::RailingMode& railingMode)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        visualInteractionSource.IsPositionXRailsEnabled(railingMode == winrt::RailingMode::Enabled);
    }
    else
    {
        visualInteractionSource.IsPositionYRailsEnabled(railingMode == winrt::RailingMode::Enabled);
    }
}

void Scroller::SetupVisualInteractionSourceChainingMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollerDimension dimension,
    const winrt::ChainingMode& chainingMode)
{
    MUX_ASSERT(visualInteractionSource);

    winrt::InteractionChainingMode interactionChainingMode = InteractionChainingModeFromChainingMode(chainingMode);

    switch (dimension)
    {
        case ScrollerDimension::HorizontalScroll:
            visualInteractionSource.PositionXChainingMode(interactionChainingMode);
            break;
        case ScrollerDimension::VerticalScroll:
            visualInteractionSource.PositionYChainingMode(interactionChainingMode);
            break;
        case ScrollerDimension::ZoomFactor:
            visualInteractionSource.ScaleChainingMode(interactionChainingMode);
            break;
        default:
            MUX_ASSERT(false);
    }
}

void Scroller::SetupVisualInteractionSourceMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollerDimension dimension,
    const winrt::ScrollMode& scrollMode)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(scrollMode == winrt::ScrollMode::Enabled || scrollMode == winrt::ScrollMode::Disabled);

    winrt::InteractionSourceMode interactionSourceMode = InteractionSourceModeFromScrollMode(scrollMode);

    switch (dimension)
    {
        case ScrollerDimension::HorizontalScroll:
            visualInteractionSource.PositionXSourceMode(interactionSourceMode);
            break;
        case ScrollerDimension::VerticalScroll:
            visualInteractionSource.PositionYSourceMode(interactionSourceMode);
            break;
        default:
            MUX_ASSERT(false);
    }
}

void Scroller::SetupVisualInteractionSourceMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    const winrt::ZoomMode& zoomMode)
{
    MUX_ASSERT(visualInteractionSource);

    visualInteractionSource.ScaleSourceMode(InteractionSourceModeFromZoomMode(zoomMode));
}

#ifdef IsMouseWheelScrollDisabled
void Scroller::SetupVisualInteractionSourcePointerWheelConfig(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollerDimension dimension,
    const winrt::ScrollMode& scrollMode)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(scrollMode == winrt::ScrollMode::Enabled || scrollMode == winrt::ScrollMode::Disabled);
    MUX_ASSERT(SharedHelpers::IsRS5OrHigher());

    winrt::InteractionSourceRedirectionMode interactionSourceRedirectionMode = InteractionSourceRedirectionModeFromScrollMode(scrollMode);

    switch (dimension)
    {
        case ScrollerDimension::HorizontalScroll:
            visualInteractionSource.PointerWheelConfig().PositionXSourceMode(interactionSourceRedirectionMode);
            break;
        case ScrollerDimension::VerticalScroll:
            visualInteractionSource.PointerWheelConfig().PositionYSourceMode(interactionSourceRedirectionMode);
            break;
        default:
            MUX_ASSERT(false);
    }
}
#endif

#ifdef IsMouseWheelZoomDisabled
void Scroller::SetupVisualInteractionSourcePointerWheelConfig(
    const winrt::VisualInteractionSource& visualInteractionSource,
    const winrt::ZoomMode& zoomMode)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());

    visualInteractionSource.PointerWheelConfig().ScaleSourceMode(InteractionSourceRedirectionModeFromZoomMode(zoomMode));
}
#endif

void Scroller::SetupVisualInteractionSourceRedirectionMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    const winrt::InputKind& ignoredInputKind)
{
    winrt::VisualInteractionSourceRedirectionMode redirectionMode = winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadOnly;

    if (SharedHelpers::AreInteractionTrackerPointerWheelRedirectionModesAvailable() &&
        !IsInputKindIgnored(winrt::InputKind::MouseWheel))
    {
        redirectionMode = winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadAndPointerWheel;
    }

    visualInteractionSource.ManipulationRedirectionMode(redirectionMode);
}

void Scroller::SetupVisualInteractionSourceCenterPointModifier(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollerDimension dimension)
{
    MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    float xamlLayoutOffset = dimension == ScrollerDimension::HorizontalScroll ? m_contentLayoutOffsetX : m_contentLayoutOffsetY;

    if (xamlLayoutOffset == 0.0f)
    {
        if (dimension == ScrollerDimension::HorizontalScroll)
        {
            visualInteractionSource.ConfigureCenterPointXModifiers(nullptr);
            m_interactionTracker.ConfigureCenterPointXInertiaModifiers(nullptr);
        }
        else
        {
            visualInteractionSource.ConfigureCenterPointYModifiers(nullptr);
            m_interactionTracker.ConfigureCenterPointYInertiaModifiers(nullptr);
        }
    }
    else
    {
        winrt::Compositor compositor = visualInteractionSource.Compositor();
        winrt::ExpressionAnimation conditionCenterPointModifier = compositor.CreateExpressionAnimation(L"true");
        winrt::CompositionConditionalValue conditionValueCenterPointModifier = winrt::CompositionConditionalValue::Create(compositor);
        winrt::ExpressionAnimation valueCenterPointModifier = compositor.CreateExpressionAnimation(
            dimension == ScrollerDimension::HorizontalScroll ?
            L"visualInteractionSource.CenterPoint.X - xamlLayoutOffset" :
            L"visualInteractionSource.CenterPoint.Y - xamlLayoutOffset");

        valueCenterPointModifier.SetReferenceParameter(L"visualInteractionSource", visualInteractionSource);
        valueCenterPointModifier.SetScalarParameter(L"xamlLayoutOffset", xamlLayoutOffset);

        conditionValueCenterPointModifier.Condition(conditionCenterPointModifier);
        conditionValueCenterPointModifier.Value(valueCenterPointModifier);

        auto centerPointModifiers = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();
        centerPointModifiers.Append(conditionValueCenterPointModifier);

        if (dimension == ScrollerDimension::HorizontalScroll)
        {
            visualInteractionSource.ConfigureCenterPointXModifiers(centerPointModifiers);
            m_interactionTracker.ConfigureCenterPointXInertiaModifiers(centerPointModifiers);
        }
        else
        {
            visualInteractionSource.ConfigureCenterPointYModifiers(centerPointModifiers);
            m_interactionTracker.ConfigureCenterPointYInertiaModifiers(centerPointModifiers);
        }
    }
}

#ifdef USE_SCROLLMODE_AUTO
winrt::ScrollMode Scroller::GetComputedScrollMode(ScrollerDimension dimension, bool ignoreZoomMode)
{
    winrt::ScrollMode oldComputedScrollMode;
    winrt::ScrollMode newComputedScrollMode;

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        oldComputedScrollMode = ComputedHorizontalScrollMode();
        newComputedScrollMode = HorizontalScrollMode();
    }
    else
    {
        MUX_ASSERT(dimension == ScrollerDimension::VerticalScroll);
        oldComputedScrollMode = ComputedVerticalScrollMode();
        newComputedScrollMode = VerticalScrollMode();
    }

    if (newComputedScrollMode == winrt::ScrollMode::Auto)
    {
        if (!ignoreZoomMode && ZoomMode() == winrt::ZoomMode::Enabled)
        {
            // Allow scrolling when zooming is turned on so that the Content does not get stuck in the given dimension
            // when it becomes smaller than the viewport.
            newComputedScrollMode = winrt::ScrollMode::Enabled;
        }
        else
        {
            if (dimension == ScrollerDimension::HorizontalScroll)
            {
                // Enable horizontal scrolling only when the Content's width is larger than the Scroller's width
                newComputedScrollMode = ScrollableWidth() > 0.0 ? winrt::ScrollMode::Enabled : winrt::ScrollMode::Disabled;
            }
            else
            {
                // Enable vertical scrolling only when the Content's height is larger than the Scroller's height
                newComputedScrollMode = ScrollableHeight() > 0.0 ? winrt::ScrollMode::Enabled : winrt::ScrollMode::Disabled;
            }
        }
    }

    if (oldComputedScrollMode != newComputedScrollMode)
    {
        if (dimension == ScrollerDimension::HorizontalScroll)
        {
            SetValue(s_ComputedHorizontalScrollModeProperty, box_value(newComputedScrollMode));
        }
        else
        {
            SetValue(s_ComputedVerticalScrollModeProperty, box_value(newComputedScrollMode));
        }
    }

    return newComputedScrollMode;
}
#endif

#ifdef IsMouseWheelScrollDisabled
winrt::ScrollMode Scroller::GetComputedMouseWheelScrollMode(ScrollerDimension dimension)
{
    MUX_ASSERT(SharedHelpers::IsRS5OrHigher());

    // TODO: c.f. Task 18569498 - Consider public IsMouseWheelHorizontalScrollDisabled/IsMouseWheelVerticalScrollDisabled properties
#ifdef USE_SCROLLMODE_AUTO
    return GetComputedScrollMode(dimension);
#else
    return dimension == ScrollerDimension::HorizontalScroll ? HorizontalScrollMode() : VerticalScrollMode();
#endif
}
#endif

#ifdef IsMouseWheelZoomDisabled
winrt::ZoomMode Scroller::GetMouseWheelZoomMode()
{
    MUX_ASSERT(Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());

    // TODO: c.f. Task 18569498 - Consider public IsMouseWheelZoomDisabled properties
    return ZoomMode();
}
#endif

double Scroller::GetComputedMaxWidth(
    double defaultMaxWidth,
    const winrt::FrameworkElement& content) const
{
    MUX_ASSERT(content);

    winrt::Thickness contentMargin = content.Margin();
    double marginWidth = contentMargin.Left + contentMargin.Right;
    double computedMaxWidth = defaultMaxWidth;
    double width = content.Width();
    double minWidth = content.MinWidth();
    double maxWidth = content.MaxWidth();

    if (!isnan(width))
    {
        width = std::max(0.0, width + marginWidth);
        computedMaxWidth = width;
    }
    if (!isnan(minWidth))
    {
        minWidth = std::max(0.0, minWidth + marginWidth);
        computedMaxWidth = std::max(computedMaxWidth, minWidth);
    }
    if (!isnan(maxWidth))
    {
        maxWidth = std::max(0.0, maxWidth + marginWidth);
        computedMaxWidth = std::min(computedMaxWidth, maxWidth);
    }

    return computedMaxWidth;
}

double Scroller::GetComputedMaxHeight(
    double defaultMaxHeight,
    const winrt::FrameworkElement& content) const
{
    MUX_ASSERT(content);

    winrt::Thickness contentMargin = content.Margin();
    double marginHeight = contentMargin.Top + contentMargin.Bottom;
    double computedMaxHeight = defaultMaxHeight;
    double height = content.Height();
    double minHeight = content.MinHeight();
    double maxHeight = content.MaxHeight();

    if (!isnan(height))
    {
        height = std::max(0.0, height + marginHeight);
        computedMaxHeight = height;
    }
    if (!isnan(minHeight))
    {
        minHeight = std::max(0.0, minHeight + marginHeight);
        computedMaxHeight = std::max(computedMaxHeight, minHeight);
    }
    if (!isnan(maxHeight))
    {
        maxHeight = std::max(0.0, maxHeight + marginHeight);
        computedMaxHeight = std::min(computedMaxHeight, maxHeight);
    }

    return computedMaxHeight;
}

// Computes the content's layout offsets at zoomFactor 1 coming from the Margin property and the difference between the extent and render sizes.
winrt::float2 Scroller::GetArrangeRenderSizesDelta(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    double deltaX = m_unzoomedExtentWidth - content.RenderSize().Width;
    double deltaY = m_unzoomedExtentHeight - content.RenderSize().Height;

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        winrt::HorizontalAlignment horizontalAlignment = contentAsFE.HorizontalAlignment();
        winrt::VerticalAlignment verticalAlignment = contentAsFE.VerticalAlignment();

        if (horizontalAlignment == winrt::HorizontalAlignment::Stretch)
        {
            deltaX = 0.0;
        }

        if (verticalAlignment == winrt::VerticalAlignment::Stretch)
        {
            deltaY = 0.0;
        }

        const winrt::Thickness contentMargin = contentAsFE.Margin();

        if (horizontalAlignment == winrt::HorizontalAlignment::Center ||
            horizontalAlignment == winrt::HorizontalAlignment::Right)
        {
            deltaX -= contentMargin.Left + contentMargin.Right;
        }

        if (verticalAlignment == winrt::VerticalAlignment::Center ||
            verticalAlignment == winrt::VerticalAlignment::Bottom)
        {
            deltaY -= contentMargin.Top + contentMargin.Bottom;
        }

        if (horizontalAlignment == winrt::HorizontalAlignment::Center ||
            horizontalAlignment == winrt::HorizontalAlignment::Stretch)
        {
            deltaX /= 2.0f;
        }
        else if (horizontalAlignment == winrt::HorizontalAlignment::Left)
        {
            deltaX = 0.0f;
        }

        if (verticalAlignment == winrt::VerticalAlignment::Center ||
            verticalAlignment == winrt::VerticalAlignment::Stretch)
        {
            deltaY /= 2.0f;
        }
        else if (verticalAlignment == winrt::VerticalAlignment::Top)
        {
            deltaY = 0.0f;
        }

        deltaX += contentMargin.Left;
        deltaY += contentMargin.Top;
    }

    return winrt::float2{ static_cast<float>(deltaX), static_cast<float>(deltaY) };
}

// Returns the expression for the m_minPositionExpressionAnimation animation based on the Content.HorizontalAlignment,
// Content.VerticalAlignment, InteractionTracker.Scale, Content arrange size (which takes Content.Margin into account) and
// ScrollerVisual.Size properties.
winrt::hstring Scroller::GetMinPositionExpression(
    const winrt::UIElement& content) const
{
    return StringUtil::FormatString(L"Vector3(%1!s!, %2!s!, 0.0f)", GetMinPositionXExpression(content).c_str(), GetMinPositionYExpression(content).c_str());
}

winrt::hstring Scroller::GetMinPositionXExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        std::wstring_view maxOffset{ L"contentSizeX * it.Scale - scrollerVisual.Size.X" };

        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"Min(0.0f, (%1!s!) / 2.0f) + contentLayoutOffsetX", maxOffset.data());
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            return StringUtil::FormatString(L"Min(0.0f, %1!s!) + contentLayoutOffsetX", maxOffset.data());
        }
    }

    return winrt::hstring(L"contentLayoutOffsetX");
}

winrt::hstring Scroller::GetMinPositionYExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        std::wstring_view maxOffset = L"contentSizeY * it.Scale - scrollerVisual.Size.Y";

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"Min(0.0f, (%1!s!) / 2.0f) + contentLayoutOffsetY", maxOffset.data());
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            return StringUtil::FormatString(L"Min(0.0f, %1!s!) + contentLayoutOffsetY", maxOffset.data());
        }
    }

    return winrt::hstring(L"contentLayoutOffsetY");
}

// Returns the expression for the m_maxPositionExpressionAnimation animation based on the Content.HorizontalAlignment,
// Content.VerticalAlignment, InteractionTracker.Scale, Content arrange size (which takes Content.Margin into account) and
// ScrollerVisual.Size properties.
winrt::hstring Scroller::GetMaxPositionExpression(
    const winrt::UIElement& content) const
{
    return StringUtil::FormatString(L"Vector3(%1!s!, %2!s!, 0.0f)", GetMaxPositionXExpression(content).c_str(), GetMaxPositionYExpression(content).c_str());
}

winrt::hstring Scroller::GetMaxPositionXExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        std::wstring_view maxOffset{ L"(contentSizeX * it.Scale - scrollerVisual.Size.X)" };

        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"%1!s! >= 0 ? %1!s! + contentLayoutOffsetX : %1!s! / 2.0f + contentLayoutOffsetX", maxOffset.data());
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            return StringUtil::FormatString(L"%1!s! + contentLayoutOffsetX", maxOffset.data());
        }
    }

    return winrt::hstring(L"Max(0.0f, contentSizeX * it.Scale - scrollerVisual.Size.X) + contentLayoutOffsetX");
}

winrt::hstring Scroller::GetMaxPositionYExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        std::wstring_view maxOffset{ L"(contentSizeY * it.Scale - scrollerVisual.Size.Y)" };

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"%1!s! >= 0 ? %1!s! + contentLayoutOffsetY : %1!s! / 2.0f + contentLayoutOffsetY", maxOffset.data());
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            return StringUtil::FormatString(L"%1!s! + contentLayoutOffsetY", maxOffset.data());
        }
    }

    return winrt::hstring(L"Max(0.0f, contentSizeY * it.Scale - scrollerVisual.Size.Y) + contentLayoutOffsetY");
}

winrt::CompositionAnimation Scroller::GetPositionAnimation(
    double zoomedHorizontalOffset,
    double zoomedVerticalOffset,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    int32_t offsetsChangeId)
{
    MUX_ASSERT(m_interactionTracker);

    int64_t minDuration = s_offsetsChangeMinMs;
    int64_t maxDuration = s_offsetsChangeMaxMs;
    int64_t unitDuration = s_offsetsChangeMsPerUnit;
    const bool isHorizontalScrollControllerRequest = static_cast<char>(operationTrigger) & static_cast<char>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest);
    const bool isVerticalScrollControllerRequest = static_cast<char>(operationTrigger) & static_cast<char>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest);
    const int64_t distance = static_cast<int64_t>(sqrt(pow(zoomedHorizontalOffset - m_zoomedHorizontalOffset, 2.0) + pow(zoomedVerticalOffset - m_zoomedVerticalOffset, 2.0)));
    const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
    winrt::Vector3KeyFrameAnimation positionAnimation = compositor.CreateVector3KeyFrameAnimation();
    com_ptr<ScrollerTestHooks> globalTestHooks = ScrollerTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        int unitDurationTestOverride;
        int minDurationTestOverride;
        int maxDurationTestOverride;

        globalTestHooks->GetOffsetsChangeVelocityParameters(unitDurationTestOverride, minDurationTestOverride, maxDurationTestOverride);

        minDuration = minDurationTestOverride;
        maxDuration = maxDurationTestOverride;
        unitDuration = unitDurationTestOverride;
    }

    winrt::float2 endPosition = ComputePositionFromOffsets(zoomedHorizontalOffset, zoomedVerticalOffset);

    positionAnimation.InsertKeyFrame(1.0f, winrt::float3(endPosition, 0.0f));
    positionAnimation.Duration(winrt::TimeSpan::duration(std::clamp(distance * unitDuration, minDuration, maxDuration) * 10000));

    winrt::float2 currentPosition{ m_interactionTracker.Position().x, m_interactionTracker.Position().y };

    if (isHorizontalScrollControllerRequest || isVerticalScrollControllerRequest)
    {
        winrt::CompositionAnimation customAnimation = nullptr;

        if (isHorizontalScrollControllerRequest && m_horizontalScrollController)
        {
            customAnimation = m_horizontalScrollController.get().GetScrollAnimation(
                winrt::ScrollInfo{ offsetsChangeId },
                currentPosition,
                positionAnimation);
        }
        if (isVerticalScrollControllerRequest && m_verticalScrollController)
        {
            customAnimation = m_verticalScrollController.get().GetScrollAnimation(
                winrt::ScrollInfo{ offsetsChangeId },
                currentPosition,
                customAnimation ? customAnimation : positionAnimation);
        }
        return customAnimation ? customAnimation : positionAnimation;
    }

    return RaiseScrollAnimationStarting(positionAnimation, currentPosition, endPosition, offsetsChangeId);
}

winrt::CompositionAnimation Scroller::GetZoomFactorAnimation(
    float zoomFactor,
    const winrt::float2& centerPoint,
    int32_t zoomFactorChangeId)
{
    int64_t minDuration = s_zoomFactorChangeMinMs;
    int64_t maxDuration = s_zoomFactorChangeMaxMs;
    int64_t unitDuration = s_zoomFactorChangeMsPerUnit;
    const int64_t distance = static_cast<int64_t>(abs(zoomFactor - m_zoomFactor));
    const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
    winrt::ScalarKeyFrameAnimation zoomFactorAnimation = compositor.CreateScalarKeyFrameAnimation();
    com_ptr<ScrollerTestHooks> globalTestHooks = ScrollerTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        int unitDurationTestOverride;
        int minDurationTestOverride;
        int maxDurationTestOverride;

        globalTestHooks->GetZoomFactorChangeVelocityParameters(unitDurationTestOverride, minDurationTestOverride, maxDurationTestOverride);

        minDuration = minDurationTestOverride;
        maxDuration = maxDurationTestOverride;
        unitDuration = unitDurationTestOverride;
    }

    zoomFactorAnimation.InsertKeyFrame(1.0f, zoomFactor);
    zoomFactorAnimation.Duration(winrt::TimeSpan::duration(std::clamp(distance * unitDuration, minDuration, maxDuration) * 10000));

    return RaiseZoomAnimationStarting(zoomFactorAnimation, zoomFactor, centerPoint, zoomFactorChangeId);
}

int Scroller::GetNextViewChangeId()
{
    return (m_latestViewChangeId == std::numeric_limits<int>::max()) ? 0 : m_latestViewChangeId + 1;
}

void Scroller::SetupPositionBoundariesExpressionAnimations(
    const winrt::UIElement& content)
{
    MUX_ASSERT(content);
    MUX_ASSERT(m_minPositionExpressionAnimation);
    MUX_ASSERT(m_maxPositionExpressionAnimation);
    MUX_ASSERT(m_interactionTracker);

    const winrt::Visual scrollerVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);

    winrt::hstring s = m_minPositionExpressionAnimation.Expression();

    if (s.empty())
    {
        m_minPositionExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_minPositionExpressionAnimation.SetReferenceParameter(L"scrollerVisual", scrollerVisual);
    }

    m_minPositionExpressionAnimation.Expression(GetMinPositionExpression(content));

    s = m_maxPositionExpressionAnimation.Expression();

    if (s.empty())
    {
        m_maxPositionExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_maxPositionExpressionAnimation.SetReferenceParameter(L"scrollerVisual", scrollerVisual);
    }

    m_maxPositionExpressionAnimation.Expression(GetMaxPositionExpression(content));

    UpdatePositionBoundaries(content);
}

void Scroller::SetupTransformExpressionAnimations(
    const winrt::UIElement& content)
{
    const bool useTranslationProperty = IsVisualTranslationPropertyAvailable();

    MUX_ASSERT(content);
    MUX_ASSERT(m_translationExpressionAnimation || !useTranslationProperty);
    MUX_ASSERT(m_transformMatrixTranslateXExpressionAnimation || useTranslationProperty);
    MUX_ASSERT(m_transformMatrixTranslateYExpressionAnimation || useTranslationProperty);
    MUX_ASSERT(m_zoomFactorExpressionAnimation || !useTranslationProperty);
    MUX_ASSERT(m_transformMatrixZoomFactorExpressionAnimation || useTranslationProperty);
    MUX_ASSERT(m_interactionTracker);

    winrt::float2 arrangeRenderSizesDelta = GetArrangeRenderSizesDelta(content);

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this,
        L"arrangeRenderSizesDelta", arrangeRenderSizesDelta.x, arrangeRenderSizesDelta.y);

    if (useTranslationProperty)
    {
        m_translationExpressionAnimation.Expression(
            L"Vector3(-it.Position.X + (it.Scale - 1.0f) * adjustment.X, -it.Position.Y + (it.Scale - 1.0f) * adjustment.Y, 0.0f)");
        m_translationExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_translationExpressionAnimation.SetVector2Parameter(L"adjustment", arrangeRenderSizesDelta);

        m_zoomFactorExpressionAnimation.Expression(L"Vector3(it.Scale, it.Scale, 1.0f)");
        m_zoomFactorExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
    }
    else
    {
        m_transformMatrixTranslateXExpressionAnimation.Expression(
            L"-it.Position.X + (it.Scale - 1.0f) * adjustment.X");
        m_transformMatrixTranslateXExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_transformMatrixTranslateXExpressionAnimation.SetVector2Parameter(L"adjustment", arrangeRenderSizesDelta);

        m_transformMatrixTranslateYExpressionAnimation.Expression(
            L"-it.Position.Y + (it.Scale - 1.0f) * adjustment.Y");
        m_transformMatrixTranslateYExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_transformMatrixTranslateYExpressionAnimation.SetVector2Parameter(L"adjustment", arrangeRenderSizesDelta);

        m_transformMatrixZoomFactorExpressionAnimation.Expression(L"it.Scale");
        m_transformMatrixZoomFactorExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
    }

    StartTransformExpressionAnimations(content);
}

void Scroller::StartTransformExpressionAnimations(
    const winrt::UIElement& content)
{
    if (content)
    {
        if (SharedHelpers::IsTranslationFacadeAvailable(content))
        {
            m_translationExpressionAnimation.Target(GetVisualTargetedPropertyName(ScrollerDimension::Scroll));
            m_zoomFactorExpressionAnimation.Target(GetVisualTargetedPropertyName(ScrollerDimension::ZoomFactor));
            content.StartAnimation(m_translationExpressionAnimation);
            content.StartAnimation(m_zoomFactorExpressionAnimation);
        }
        else
        {
            const winrt::Visual contentVisual = winrt::ElementCompositionPreview::GetElementVisual(content);
            if (IsVisualTranslationPropertyAvailable())
            {
                winrt::ElementCompositionPreview::SetIsTranslationEnabled(content, true);
                contentVisual.StartAnimation(GetVisualTargetedPropertyName(ScrollerDimension::Scroll), m_translationExpressionAnimation);
                contentVisual.StartAnimation(GetVisualTargetedPropertyName(ScrollerDimension::ZoomFactor), m_zoomFactorExpressionAnimation);
            }
            else
            {
                contentVisual.StartAnimation(GetVisualTargetedPropertyName(ScrollerDimension::HorizontalScroll), m_transformMatrixTranslateXExpressionAnimation);
                contentVisual.StartAnimation(GetVisualTargetedPropertyName(ScrollerDimension::VerticalScroll), m_transformMatrixTranslateYExpressionAnimation);
                contentVisual.StartAnimation(GetVisualTargetedPropertyName(ScrollerDimension::HorizontalZoomFactor), m_transformMatrixZoomFactorExpressionAnimation);
                contentVisual.StartAnimation(GetVisualTargetedPropertyName(ScrollerDimension::VerticalZoomFactor), m_transformMatrixZoomFactorExpressionAnimation);
            }
        }
    }
}

void Scroller::StopTransformExpressionAnimations(
    const winrt::UIElement& content)
{
    if (content)
    {
        if (SharedHelpers::IsTranslationFacadeAvailable(content))
        {
            content.StopAnimation(m_translationExpressionAnimation);
            content.StopAnimation(m_zoomFactorExpressionAnimation);
        }
        else
        {
            const winrt::Visual contentVisual = winrt::ElementCompositionPreview::GetElementVisual(content);
            if (IsVisualTranslationPropertyAvailable())
            {
                contentVisual.StopAnimation(GetVisualTargetedPropertyName(ScrollerDimension::Scroll));
                contentVisual.StopAnimation(GetVisualTargetedPropertyName(ScrollerDimension::ZoomFactor));
            }
            else
            {
                contentVisual.StopAnimation(GetVisualTargetedPropertyName(ScrollerDimension::HorizontalScroll));
                contentVisual.StopAnimation(GetVisualTargetedPropertyName(ScrollerDimension::VerticalScroll));
                contentVisual.StopAnimation(GetVisualTargetedPropertyName(ScrollerDimension::HorizontalZoomFactor));
                contentVisual.StopAnimation(GetVisualTargetedPropertyName(ScrollerDimension::VerticalZoomFactor));
            }
        }
    }
}

void Scroller::StartExpressionAnimationSourcesAnimations()
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(m_expressionAnimationSources);
    MUX_ASSERT(m_positionSourceExpressionAnimation);
    MUX_ASSERT(m_minPositionSourceExpressionAnimation);
    MUX_ASSERT(m_maxPositionSourceExpressionAnimation);
    MUX_ASSERT(m_zoomFactorSourceExpressionAnimation);

    m_expressionAnimationSources.StartAnimation(s_positionSourcePropertyName, m_positionSourceExpressionAnimation);
    m_expressionAnimationSources.StartAnimation(s_minPositionSourcePropertyName, m_minPositionSourceExpressionAnimation);
    m_expressionAnimationSources.StartAnimation(s_maxPositionSourcePropertyName, m_maxPositionSourceExpressionAnimation);
    m_expressionAnimationSources.StartAnimation(s_zoomFactorSourcePropertyName, m_zoomFactorSourceExpressionAnimation);
}

void Scroller::StopExpressionAnimationSourcesAnimations()
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(m_expressionAnimationSources);
    MUX_ASSERT(m_positionSourceExpressionAnimation);
    MUX_ASSERT(m_minPositionSourceExpressionAnimation);
    MUX_ASSERT(m_maxPositionSourceExpressionAnimation);
    MUX_ASSERT(m_zoomFactorSourceExpressionAnimation);

    //m_expressionAnimationSources.StopAnimation(s_offsetSourcePropertyName);
    m_expressionAnimationSources.StopAnimation(s_positionSourcePropertyName);
    m_expressionAnimationSources.StopAnimation(s_minPositionSourcePropertyName);
    m_expressionAnimationSources.StopAnimation(s_maxPositionSourcePropertyName);
    m_expressionAnimationSources.StopAnimation(s_zoomFactorSourcePropertyName);
}

void Scroller::StartScrollControllerExpressionAnimationSourcesAnimations(
    ScrollerDimension dimension)
{
    MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        MUX_ASSERT(m_horizontalScrollControllerExpressionAnimationSources);
        MUX_ASSERT(m_horizontalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(m_horizontalScrollControllerMaxOffsetExpressionAnimation);

        m_horizontalScrollControllerExpressionAnimationSources.StartAnimation(s_offsetPropertyName, m_horizontalScrollControllerOffsetExpressionAnimation);
        m_horizontalScrollControllerExpressionAnimationSources.StartAnimation(s_maxOffsetPropertyName, m_horizontalScrollControllerMaxOffsetExpressionAnimation);
    }
    else
    {
        MUX_ASSERT(m_verticalScrollControllerExpressionAnimationSources);
        MUX_ASSERT(m_verticalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(m_verticalScrollControllerMaxOffsetExpressionAnimation);

        m_verticalScrollControllerExpressionAnimationSources.StartAnimation(s_offsetPropertyName, m_verticalScrollControllerOffsetExpressionAnimation);
        m_verticalScrollControllerExpressionAnimationSources.StartAnimation(s_maxOffsetPropertyName, m_verticalScrollControllerMaxOffsetExpressionAnimation);
    }
}

void Scroller::StopScrollControllerExpressionAnimationSourcesAnimations(
    ScrollerDimension dimension)
{
    MUX_ASSERT(SharedHelpers::IsRS2OrHigher());
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        MUX_ASSERT(m_horizontalScrollControllerExpressionAnimationSources);

        m_horizontalScrollControllerExpressionAnimationSources.StopAnimation(s_offsetPropertyName);
        m_horizontalScrollControllerExpressionAnimationSources.StopAnimation(s_maxOffsetPropertyName);
    }
    else
    {
        MUX_ASSERT(m_verticalScrollControllerExpressionAnimationSources);

        m_verticalScrollControllerExpressionAnimationSources.StopAnimation(s_offsetPropertyName);
        m_verticalScrollControllerExpressionAnimationSources.StopAnimation(s_maxOffsetPropertyName);
    }
}

winrt::InteractionChainingMode Scroller::InteractionChainingModeFromChainingMode(
    const winrt::ChainingMode& chainingMode)
{
    switch (chainingMode)
    {
        case winrt::ChainingMode::Always:
            return winrt::InteractionChainingMode::Always;
        case winrt::ChainingMode::Auto:
            return winrt::InteractionChainingMode::Auto;
        default:
            return winrt::InteractionChainingMode::Never;
    }
}

#ifdef IsMouseWheelScrollDisabled
winrt::InteractionSourceRedirectionMode Scroller::InteractionSourceRedirectionModeFromScrollMode(
    const winrt::ScrollMode& scrollMode)
{
    MUX_ASSERT(SharedHelpers::IsRS5OrHigher());
    MUX_ASSERT(scrollMode == winrt::ScrollMode::Enabled || scrollMode == winrt::ScrollMode::Disabled);

    return scrollMode == winrt::ScrollMode::Enabled ? winrt::InteractionSourceRedirectionMode::Enabled : winrt::InteractionSourceRedirectionMode::Disabled;
}
#endif

#ifdef IsMouseWheelZoomDisabled
winrt::InteractionSourceRedirectionMode Scroller::InteractionSourceRedirectionModeFromZoomMode(
    const winrt::ZoomMode& zoomMode)
{
    MUX_ASSERT(Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());

    return zoomMode == winrt::ZoomMode::Enabled ? winrt::InteractionSourceRedirectionMode::Enabled : winrt::InteractionSourceRedirectionMode::Disabled;
}
#endif

winrt::InteractionSourceMode Scroller::InteractionSourceModeFromScrollMode(
    const winrt::ScrollMode& scrollMode)
{
    return scrollMode == winrt::ScrollMode::Enabled ? winrt::InteractionSourceMode::EnabledWithInertia : winrt::InteractionSourceMode::Disabled;
}

winrt::InteractionSourceMode Scroller::InteractionSourceModeFromZoomMode(
    const winrt::ZoomMode& zoomMode)
{
    return zoomMode == winrt::ZoomMode::Enabled ? winrt::InteractionSourceMode::EnabledWithInertia : winrt::InteractionSourceMode::Disabled;
}

double Scroller::ComputeZoomedOffsetWithMinimalChange(
    double viewportStart,
    double viewportEnd,
    double childStart,
    double childEnd)
{
    bool above = childStart < viewportStart && childEnd < viewportEnd;
    bool below = childEnd > viewportEnd && childStart > viewportStart;
    bool larger = (childEnd - childStart) > (viewportEnd - viewportStart);

    // # CHILD POSITION   CHILD SIZE   SCROLL   REMEDY
    // 1 Above viewport   <= viewport  Down     Align top edge of content & viewport
    // 2 Above viewport   >  viewport  Down     Align bottom edge of content & viewport
    // 3 Below viewport   <= viewport  Up       Align bottom edge of content & viewport
    // 4 Below viewport   >  viewport  Up       Align top edge of content & viewport
    // 5 Entirely within viewport      NA       No change
    // 6 Spanning viewport             NA       No change
    if ((above && !larger) || (below && larger))
    {
        // Cases 1 & 4
        return childStart;
    }
    else if (above || below)
    {
        // Cases 2 & 3
        return childEnd - viewportEnd + viewportStart;
    }

    // cases 5 & 6
    return viewportStart;
}

winrt::Rect Scroller::GetDescendantBounds(
    const winrt::UIElement& content,
    const winrt::UIElement& descendant,
    const winrt::Rect& descendantRect)
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();
    const winrt::GeneralTransform transform = descendant.TransformToVisual(content);
    winrt::Thickness contentMargin{};

    if (contentAsFE)
    {
        contentMargin = contentAsFE.Margin();
    }

    return transform.TransformBounds(winrt::Rect{
        static_cast<float>(contentMargin.Left + descendantRect.X),
        static_cast<float>(contentMargin.Top + descendantRect.Y),
        descendantRect.Width,
        descendantRect.Height });
}

bool Scroller::IsZoomFactorBoundaryValid(
    double value)
{
    return !isnan(value) && isfinite(value);
}

void Scroller::ValidateZoomFactoryBoundary(double value)
{
    if (!IsZoomFactorBoundaryValid(value))
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

// Returns False prior to RS5 where the InteractionTracker does not support off-thread mouse-wheel-based zooming.
// Starting with RS5, returns True unless a test hook is set to disable the use of the InteractionTracker's built-in feature.
bool Scroller::IsInteractionTrackerMouseWheelZoomingEnabled()
{
    bool isInteractionTrackerMouseWheelZoomingEnabled = SharedHelpers::IsRS5OrHigher();

    if (isInteractionTrackerMouseWheelZoomingEnabled)
    {
        com_ptr<ScrollerTestHooks> globalTestHooks = ScrollerTestHooks::GetGlobalTestHooks();

        isInteractionTrackerMouseWheelZoomingEnabled = !globalTestHooks || globalTestHooks->IsInteractionTrackerMouseWheelZoomingEnabled();
    }

    return isInteractionTrackerMouseWheelZoomingEnabled;
}

// Returns True on RedStone 2 and later versions, where the ElementCompositionPreview::SetIsTranslationEnabled method is available.
bool Scroller::IsVisualTranslationPropertyAvailable()
{
    return DownlevelHelper::SetIsTranslationEnabledExists();
}

// Returns the target property path, according to the availability of the ElementCompositionPreview::SetIsTranslationEnabled method,
// and the provided dimension.
wstring_view Scroller::GetVisualTargetedPropertyName(ScrollerDimension dimension)
{
    switch (dimension)
    {
        case ScrollerDimension::Scroll:
            MUX_ASSERT(IsVisualTranslationPropertyAvailable());
            return s_translationPropertyName;
        case ScrollerDimension::HorizontalScroll:
            MUX_ASSERT(!IsVisualTranslationPropertyAvailable());
            return s_transformMatrixTranslateXPropertyName;
        case ScrollerDimension::VerticalScroll:
            MUX_ASSERT(!IsVisualTranslationPropertyAvailable());
            return s_transformMatrixTranslateYPropertyName;
        case ScrollerDimension::HorizontalZoomFactor:
            MUX_ASSERT(!IsVisualTranslationPropertyAvailable());
            return s_transformMatrixScaleXPropertyName;
        case ScrollerDimension::VerticalZoomFactor:
            MUX_ASSERT(!IsVisualTranslationPropertyAvailable());
            return s_transformMatrixScaleYPropertyName;
        default:
            MUX_ASSERT(dimension == ScrollerDimension::ZoomFactor);
            MUX_ASSERT(IsVisualTranslationPropertyAvailable());
            return s_scalePropertyName;
    }
}

// Invoked by both Scroller and ScrollViewer controls
bool Scroller::IsAnchorRatioValid(
    double value)
{
    return isnan(value) || (isfinite(value) && value >= 0.0 && value <= 1.0);
}

void Scroller::ValidateAnchorRatio(double value)
{
    if (!IsAnchorRatioValid(value))
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

bool Scroller::IsElementValidAnchor(
    const winrt::UIElement& element)
{
    return IsElementValidAnchor(element, Content());
}

// Invoked by ScrollerTestHooks
void Scroller::SetContentLayoutOffsetX(float contentLayoutOffsetX)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, contentLayoutOffsetX, m_contentLayoutOffsetX);

    if (m_contentLayoutOffsetX != contentLayoutOffsetX)
    {
        UpdateOffset(ScrollerDimension::HorizontalScroll, m_zoomedHorizontalOffset + contentLayoutOffsetX - m_contentLayoutOffsetX);
        m_contentLayoutOffsetX = contentLayoutOffsetX;
        InvalidateArrange();
        OnContentLayoutOffsetChanged(ScrollerDimension::HorizontalScroll);
        OnViewChanged(true /*horizontalOffsetChanged*/, false /*verticalOffsetChanged*/);
    }
}

void Scroller::SetContentLayoutOffsetY(float contentLayoutOffsetY)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, contentLayoutOffsetY, m_contentLayoutOffsetY);

    if (m_contentLayoutOffsetY != contentLayoutOffsetY)
    {
        UpdateOffset(ScrollerDimension::VerticalScroll, m_zoomedVerticalOffset + contentLayoutOffsetY - m_contentLayoutOffsetY);
        m_contentLayoutOffsetY = contentLayoutOffsetY;
        InvalidateArrange();
        OnContentLayoutOffsetChanged(ScrollerDimension::VerticalScroll);
        OnViewChanged(false /*horizontalOffsetChanged*/, true /*verticalOffsetChanged*/);
    }
}

winrt::IVector<winrt::ScrollSnapPointBase> Scroller::GetConsolidatedScrollSnapPoints(ScrollerDimension dimension)
{
    winrt::IVector<winrt::ScrollSnapPointBase> snapPoints = winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    std::set<winrt::ScrollSnapPointBase, winrtProjectionComparator> snapPointsSet;
    switch (dimension)
    {
    case ScrollerDimension::VerticalScroll :
        snapPointsSet = m_sortedConsolidatedVerticalSnapPoints;
        break;
    case ScrollerDimension::HorizontalScroll:
        snapPointsSet = m_sortedConsolidatedHorizontalSnapPoints;
        break;
    default:
        MUX_ASSERT(false);
    }

    for (winrt::ScrollSnapPointBase snapPoint : snapPointsSet)
    {
        snapPoints.Append(snapPoint);
    }
    return snapPoints;
}

winrt::IVector<winrt::ZoomSnapPointBase> Scroller::GetConsolidatedZoomSnapPoints()
{
    winrt::IVector<winrt::ZoomSnapPointBase> snapPoints = winrt::make<Vector<winrt::ZoomSnapPointBase>>();

    for (winrt::ZoomSnapPointBase snapPoint : m_sortedConsolidatedZoomSnapPoints)
    {
        snapPoints.Append(snapPoint);
    }
    return snapPoints;
}

// Invoked when a dependency property of this Scroller has changed.
void Scroller::OnPropertyChanged(
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto dependencyProperty = args.Property();

#ifdef _DEBUG
    SCROLLER_TRACE_VERBOSE(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToString(dependencyProperty).c_str());
#endif

    if (dependencyProperty == s_ContentProperty)
    {
        const winrt::IInspectable oldContent = args.OldValue();
        const winrt::IInspectable newContent = args.NewValue();
        UpdateContent(safe_cast<winrt::UIElement>(oldContent), safe_cast<winrt::UIElement>(newContent));
    }
    else if (dependencyProperty == s_BackgroundProperty)
    {
        winrt::Panel thisAsPanel = *this;

        thisAsPanel.Background(safe_cast<winrt::Brush>(args.NewValue()));
    }
    else if (dependencyProperty == s_MinZoomFactorProperty || dependencyProperty == s_MaxZoomFactorProperty)
    {
        MUX_ASSERT(IsZoomFactorBoundaryValid(unbox_value<double>(args.OldValue())));

        if (m_interactionTracker)
        {
            SetupInteractionTrackerZoomFactorBoundaries(
                MinZoomFactor(),
                MaxZoomFactor());
        }
    }
    else if (dependencyProperty == s_ContentOrientationProperty)
    {
        m_contentOrientation = ContentOrientation();

        // Raise configuration changed only when effective viewport
        // support is not available.
        if (!SharedHelpers::IsRS5OrHigher())
        {
            RaiseConfigurationChanged();
        }

        InvalidateMeasure();
    }
    else if (dependencyProperty == s_HorizontalAnchorRatioProperty ||
        dependencyProperty == s_VerticalAnchorRatioProperty)
    {
        MUX_ASSERT(IsAnchorRatioValid(unbox_value<double>(args.OldValue())));

        m_isAnchorElementDirty = true;
    }
    else if (m_scrollerVisualInteractionSource)
    {
        if (dependencyProperty == s_HorizontalScrollChainingModeProperty)
        {
            SetupVisualInteractionSourceChainingMode(
                m_scrollerVisualInteractionSource,
                ScrollerDimension::HorizontalScroll,
                HorizontalScrollChainingMode());
        }
        else if (dependencyProperty == s_VerticalScrollChainingModeProperty)
        {
            SetupVisualInteractionSourceChainingMode(
                m_scrollerVisualInteractionSource,
                ScrollerDimension::VerticalScroll,
                VerticalScrollChainingMode());
        }
        else if (dependencyProperty == s_ZoomChainingModeProperty)
        {
            SetupVisualInteractionSourceChainingMode(
                m_scrollerVisualInteractionSource,
                ScrollerDimension::ZoomFactor,
                ZoomChainingMode());
        }
        else if (dependencyProperty == s_HorizontalScrollRailingModeProperty)
        {
            SetupVisualInteractionSourceRailingMode(
                m_scrollerVisualInteractionSource,
                ScrollerDimension::HorizontalScroll,
                HorizontalScrollRailingMode());
        }
        else if (dependencyProperty == s_VerticalScrollRailingModeProperty)
        {
            SetupVisualInteractionSourceRailingMode(
                m_scrollerVisualInteractionSource,
                ScrollerDimension::VerticalScroll,
                VerticalScrollRailingMode());
        }
        else if (dependencyProperty == s_HorizontalScrollModeProperty)
        {
            UpdateVisualInteractionSourceMode(
                ScrollerDimension::HorizontalScroll);
        }
        else if (dependencyProperty == s_VerticalScrollModeProperty)
        {
            UpdateVisualInteractionSourceMode(
                ScrollerDimension::VerticalScroll);
        }
        else if (dependencyProperty == s_ZoomModeProperty)
        {
#ifdef USE_SCROLLMODE_AUTO
            // Updating the horizontal and vertical scroll modes because GetComputedScrollMode is function of ZoomMode.
            UpdateVisualInteractionSourceMode(
                ScrollerDimension::HorizontalScroll);
            UpdateVisualInteractionSourceMode(
                ScrollerDimension::VerticalScroll);
#endif

            SetupVisualInteractionSourceMode(
                m_scrollerVisualInteractionSource,
                ZoomMode());

#ifdef IsMouseWheelZoomDisabled
            if (Scroller::IsInteractionTrackerMouseWheelZoomingEnabled())
            {
                SetupVisualInteractionSourcePointerWheelConfig(
                    m_scrollerVisualInteractionSource,
                    GetMouseWheelZoomMode());
            }
#endif
        }
        else if (dependencyProperty == s_IgnoredInputKindProperty)
        {
            UpdateKeyEvents();
            UpdateManipulationRedirectionMode();
        }
    }
}

void Scroller::OnContentPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    const winrt::UIElement content = Content();

    if (content)
    {
        if (args == winrt::FrameworkElement::HorizontalAlignmentProperty() ||
            args == winrt::FrameworkElement::VerticalAlignmentProperty())
        {
            // The ExtentWidth and ExtentHeight may have to be updated because of this alignment change.
            InvalidateMeasure();

            if (m_interactionTracker)
            {
                if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
                {
                    SetupPositionBoundariesExpressionAnimations(content);
                }

                const bool useTranslationProperty = IsVisualTranslationPropertyAvailable();

                if ((!useTranslationProperty && m_transformMatrixTranslateXExpressionAnimation && m_transformMatrixTranslateYExpressionAnimation && m_transformMatrixZoomFactorExpressionAnimation) ||
                    (useTranslationProperty && m_translationExpressionAnimation && m_zoomFactorExpressionAnimation))
                {
                    SetupTransformExpressionAnimations(content);
                }
            }
        }
    }
}

void Scroller::OnCompositionTargetRendering(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    bool unhookCompositionTargetRendering = true;

    if (!m_interactionTrackerAsyncOperations.empty() && IsLoaded())
    {
        for (auto operationsIter = m_interactionTrackerAsyncOperations.begin(); operationsIter != m_interactionTrackerAsyncOperations.end();)
        {
            auto& interactionTrackerAsyncOperation = *operationsIter;

            operationsIter++;

            if (interactionTrackerAsyncOperation->IsDelayed())
            {
                interactionTrackerAsyncOperation->SetIsDelayed(false);
                unhookCompositionTargetRendering = false;
                MUX_ASSERT(interactionTrackerAsyncOperation->IsQueued());
            }
            else if (interactionTrackerAsyncOperation->IsQueued())
            {
                bool needsProcessing = false;

                interactionTrackerAsyncOperation->TickQueuedOperation(&needsProcessing);
                if (needsProcessing)
                {
                    // InteractionTracker is ready for the operation's processing.
                    ProcessDequeuedViewChange(interactionTrackerAsyncOperation);
                    if (!interactionTrackerAsyncOperation->IsAnimated())
                    {
                        unhookCompositionTargetRendering = false;
                    }
                }
                else
                {
                    unhookCompositionTargetRendering = false;
                }
            }
            else if (!interactionTrackerAsyncOperation->IsAnimated())
            {
                bool needsCompletion = false;

                interactionTrackerAsyncOperation->TickNonAnimatedOperation(&needsCompletion);
                if (needsCompletion)
                {
                    // The non-animated view change request did not result in a status change or ValuesChanged notification. Consider it completed.
                    CompleteViewChange(interactionTrackerAsyncOperation, ScrollerViewChangeResult::Completed);
                    m_interactionTrackerAsyncOperations.remove(interactionTrackerAsyncOperation);
                }
                else
                {
                    unhookCompositionTargetRendering = false;
                }
            }
        }
    }

    if (unhookCompositionTargetRendering)
    {
        UnhookCompositionTargetRendering();
    }
}

void Scroller::OnLoaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    MUX_ASSERT(!SharedHelpers::IsTH2OrLower());

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    SetupInteractionTrackerBoundaries();

    EnsureScrollerVisualInteractionSource();
    SetupScrollerVisualInteractionSource();

    if (SharedHelpers::IsRS2OrHigher())
    {
        SetupScrollControllerVisualInterationSource(ScrollerDimension::HorizontalScroll);
        SetupScrollControllerVisualInterationSource(ScrollerDimension::VerticalScroll);

        if (m_horizontalScrollControllerExpressionAnimationSources)
        {
            m_horizontalScrollController.get().SetExpressionAnimationSources(
                m_horizontalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
        if (m_verticalScrollControllerExpressionAnimationSources)
        {
            m_verticalScrollController.get().SetExpressionAnimationSources(
                m_verticalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }

    const winrt::UIElement content = Content();

    if (content)
    {
        const bool useTranslationProperty = IsVisualTranslationPropertyAvailable();

        if (((!m_transformMatrixTranslateXExpressionAnimation || !m_transformMatrixTranslateYExpressionAnimation || !m_transformMatrixZoomFactorExpressionAnimation) && !useTranslationProperty) ||
            ((!m_translationExpressionAnimation || !m_zoomFactorExpressionAnimation) && useTranslationProperty))
        {
            EnsureTransformExpressionAnimations();
            SetupTransformExpressionAnimations(content);
        }

        // Process the potentially delayed operation in the OnCompositionTargetRendering handler.
        HookCompositionTargetRendering();
    }
}

void Scroller::OnUnloaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!IsLoaded())
    {
        MUX_ASSERT(RenderSize().Width == 0.0);
        MUX_ASSERT(RenderSize().Height == 0.0);

        // All potential pending operations are interrupted when the Scroller unloads.
        CompleteInteractionTrackerOperations(
            -1 /*requestId*/,
            ScrollerViewChangeResult::Interrupted /*operationResult*/,
            ScrollerViewChangeResult::Ignored     /*unused priorNonAnimatedOperationsResult*/,
            ScrollerViewChangeResult::Ignored     /*unused priorAnimatedOperationsResult*/,
            true  /*completeOperation*/,
            false /*completePriorNonAnimatedOperations*/,
            false /*completePriorAnimatedOperations*/);

        // Unhook the potential OnCompositionTargetRendering handler since there are no pending operations.
        UnhookCompositionTargetRendering();

        const winrt::UIElement content = Content();

        UpdateUnzoomedExtentAndViewport(
            content ? m_unzoomedExtentWidth : 0.0,
            content ? m_unzoomedExtentHeight : 0.0,
            0.0 /*viewportWidth*/, 0.0 /*viewportHeight*/);
    }
}

// UIElement.PointerWheelChanged event handler for support of mouse-wheel-triggered zooming.
void Scroller::OnPointerWheelChangedHandler(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    MUX_ASSERT(!Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());

    if (!m_interactionTracker || !m_scrollerVisualInteractionSource)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"No InteractionTracker");

        // No InteractionTracker has been set up.
        return;
    }

    if (IsInputKindIgnored(winrt::InputKind::MouseWheel))
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"InputKind::MouseWheel ignored");

        // MouseWheel input is ignored.
        return;
    }

    winrt::CoreVirtualKeyStates ctrlState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Control);
    winrt::PointerPoint pointerPoint = args.GetCurrentPoint(*this);
    winrt::PointerPointProperties pointerPointProperties = pointerPoint.Properties();
    bool isHorizontalMouseWheel = pointerPointProperties.IsHorizontalMouseWheel();
    bool isControlPressed = (ctrlState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    if (!isControlPressed || isHorizontalMouseWheel)
    {
        // Mouse-wheel-triggered zooming is only attempted when Control key is down and event is not for a horizontal scroll.
        return;
    }

    if (ZoomMode() == winrt::ZoomMode::Disabled)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"ZoomMode off");

        return;
    }

    int32_t mouseWheelDelta = pointerPointProperties.MouseWheelDelta();
    float endOfInertiaZoomFactor = ComputeEndOfInertiaZoomFactor();
    float minZoomFactor = m_interactionTracker.MinScale();
    float maxZoomFactor = m_interactionTracker.MaxScale();

    if ((endOfInertiaZoomFactor == minZoomFactor && mouseWheelDelta < 0) ||
        (endOfInertiaZoomFactor == maxZoomFactor && mouseWheelDelta > 0))
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Cannot zoom beyond boundary");

        return;
    }

    // Attempt to find a zoom factor change with velocity request for mouse wheel input within the same tick.
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationWithAdditionalVelocity(
        false /*isOperationTypeForOffsetsChange*/,
        InteractionTrackerAsyncOperationTrigger::MouseWheel);

    // Mouse wheel delta amount required per initial velocity unit
    int32_t mouseWheelDeltaForVelocityUnit = s_mouseWheelDeltaForVelocityUnit;
    com_ptr<ScrollerTestHooks> globalTestHooks = ScrollerTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        mouseWheelDeltaForVelocityUnit = globalTestHooks->MouseWheelDeltaForVelocityUnit();
    }

    // Maximum absolute velocity. Any additional velocity has no effect
    const float c_maxVelocityUnits = 5.0f;
    // Incremental zoom factor change per velocity unit
    const float c_zoomFactorChangePerVelocityUnit = 0.1f;

    std::shared_ptr<ZoomFactorChangeWithAdditionalVelocity> zoomFactorChangeWithAdditionalVelocity = nullptr;
    float zoomFactorVelocity = static_cast<float>(mouseWheelDelta) / mouseWheelDeltaForVelocityUnit;

    if (interactionTrackerAsyncOperation)
    {
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();
        zoomFactorChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<ZoomFactorChangeWithAdditionalVelocity>(viewChangeBase);

        if (zoomFactorChangeWithAdditionalVelocity)
        {
            zoomFactorVelocity += zoomFactorChangeWithAdditionalVelocity->ZoomFactorVelocity();
        }
    }

    if (zoomFactorVelocity > 0.0f)
    {
        MUX_ASSERT(endOfInertiaZoomFactor < maxZoomFactor);

        // No point in exceeding the maximum effective velocity
        zoomFactorVelocity = std::min(c_maxVelocityUnits, zoomFactorVelocity);

        // Do not attempt to zoom factor beyond the MaxZoomFactor value
        zoomFactorVelocity = std::min((maxZoomFactor - endOfInertiaZoomFactor) / c_zoomFactorChangePerVelocityUnit, zoomFactorVelocity);
    }
    else
    {
        MUX_ASSERT(endOfInertiaZoomFactor > minZoomFactor);

        // No point in exceeding the minimum effective velocity
        zoomFactorVelocity = std::max(-c_maxVelocityUnits, zoomFactorVelocity);

        // Do not attempt to zoom factor beyond the MinZoomFactor value
        zoomFactorVelocity = std::max((minZoomFactor - endOfInertiaZoomFactor) / c_zoomFactorChangePerVelocityUnit, zoomFactorVelocity);
    }

    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        interactionTrackerAsyncOperation ? L"Coalesced MouseWheelDelta for zooming" : L"New MouseWheelDelta for zooming",
        mouseWheelDelta);

    if (!interactionTrackerAsyncOperation)
    {
        // Minimum absolute velocity. Any lower velocity has no effect
        const float c_minVelocityUnits = 0.05f;

        // Inertia decay rate to achieve the c_zoomFactorChangePerVelocityUnit change per velocity unit
        float mouseWheelInertiaDecayRate = SharedHelpers::IsRS2OrHigher() ? s_mouseWheelInertiaDecayRate : s_mouseWheelInertiaDecayRateRS1;

        if (globalTestHooks)
        {
            mouseWheelInertiaDecayRate = globalTestHooks->MouseWheelInertiaDecayRate();
        }

        // Make sure the initial velocity is larger than the minimum effective velocity
        zoomFactorVelocity += (zoomFactorVelocity > 0.0f) ? c_minVelocityUnits : -c_minVelocityUnits;

        winrt::IInspectable inertiaDecayRateAsInsp = box_value(mouseWheelInertiaDecayRate);
        winrt::IReference<float> inertiaDecayRate = inertiaDecayRateAsInsp.as<winrt::IReference<float>>();
        winrt::float2 centerPoint = ComputeCenterPointerForMouseWheelZooming(Content(), pointerPoint.Position());

        int32_t viewChangeId = -1;

        // Queue up a zooming with additional velocity operation
        ChangeZoomFactorWithAdditionalVelocityPrivate(
            zoomFactorVelocity,
            centerPoint,
            inertiaDecayRate,
            InteractionTrackerAsyncOperationTrigger::MouseWheel,
            &viewChangeId);
    }
    else if (zoomFactorChangeWithAdditionalVelocity)
    {
        zoomFactorChangeWithAdditionalVelocity->ZoomFactorVelocity(zoomFactorVelocity);
    }

    args.Handled(true);
}

// UIElement.BringIntoViewRequested event handler to bring an element into the viewport.
void Scroller::OnBringIntoViewRequestedHandler(
    const winrt::IInspectable& /*sender*/,
    const winrt::BringIntoViewRequestedEventArgs& args)
{
    SCROLLER_TRACE_INFO(*this, L"%s[0x%p](AnimationDesired:%d, Handled:%d, H/V AlignmentRatio:%lf,%lf, H/V Offset:%f,%f, TargetRect:%s, TargetElement:0x%p)\n",
        METH_NAME, this,
        args.AnimationDesired(), args.Handled(),
        args.HorizontalAlignmentRatio(), args.VerticalAlignmentRatio(),
        args.HorizontalOffset(), args.VerticalOffset(),
        TypeLogging::RectToString(args.TargetRect()).c_str(), args.TargetElement());

    winrt::UIElement content = Content();

    if (SharedHelpers::IsTH2OrLower() ||
        args.Handled() ||
        args.TargetElement() == static_cast<winrt::UIElement>(*this) ||
        (args.TargetElement() == content && content.Visibility() == winrt::Visibility::Collapsed) ||
        !SharedHelpers::IsAncestor(args.TargetElement(), content, true /*checkVisibility*/))
    {
        // Ignore the request when:
        // - There is no InteractionTracker to fulfill it.
        // - It was handled already.
        // - The target element is this Scroller itself. A parent scroller may fulfill the request instead then.
        // - The target element is effectively collapsed within the Scroller.
        return;
    }

    winrt::Rect targetRect{};
    int32_t offsetsChangeId = -1;
    double targetZoomedHorizontalOffset = 0.0;
    double targetZoomedVerticalOffset = 0.0;
    double appliedOffsetX = 0.0;
    double appliedOffsetY = 0.0;
    winrt::SnapPointsMode snapPointsMode = winrt::SnapPointsMode::Ignore;

    // Compute the target offsets based on the provided BringIntoViewRequestedEventArgs.
    ComputeBringIntoViewTargetOffsets(
        content,
        snapPointsMode,
        args,
        &targetZoomedHorizontalOffset,
        &targetZoomedVerticalOffset,
        &appliedOffsetX,
        &appliedOffsetY,
        &targetRect);

    if (HasBringingIntoViewListener())
    {
        // Raise the Scroller.BringingIntoView event to give the listeners a chance to adjust the operation.

        offsetsChangeId = m_latestViewChangeId = GetNextViewChangeId();

        if (!RaiseBringingIntoView(
            targetZoomedHorizontalOffset,
            targetZoomedVerticalOffset,
            args,
            offsetsChangeId,
            &snapPointsMode))
        {
            // A listener canceled the operation in the Scroller.BringingIntoView event handler before any scrolling was attempted.
            RaiseViewChangeCompleted(true /*isForScroll*/, ScrollerViewChangeResult::Completed, offsetsChangeId);
            return;
        }

        content = Content();

        if (!content ||
            args.Handled() ||
            args.TargetElement() == static_cast<winrt::UIElement>(*this) ||
            (args.TargetElement() == content && content.Visibility() == winrt::Visibility::Collapsed) ||
            !SharedHelpers::IsAncestor(args.TargetElement(), content, true /*checkVisibility*/))
        {
            // Again, ignore the request when:
            // - There is no Content anymore.
            // - The request was handled already.
            // - The target element is this Scroller itself. A parent scroller may fulfill the request instead then.
            // - The target element is effectively collapsed within the Scroller.
            return;
        }

        // Re-evaluate the target offsets based on the potentially modified BringIntoViewRequestedEventArgs.
        // Take into account potential SnapPointsMode == Default so that parents contribute accordingly.
        ComputeBringIntoViewTargetOffsets(
            content,
            snapPointsMode,
            args,
            &targetZoomedHorizontalOffset,
            &targetZoomedVerticalOffset,
            &appliedOffsetX,
            &appliedOffsetY,
            &targetRect);
    }

    // Do not include the applied offsets so that potential parent bring-into-view contributors ignore that shift.
    winrt::Rect nextTargetRect{
        static_cast<float>(targetRect.X * m_zoomFactor - targetZoomedHorizontalOffset - appliedOffsetX),
        static_cast<float>(targetRect.Y * m_zoomFactor - targetZoomedVerticalOffset - appliedOffsetY),
        std::min(targetRect.Width * m_zoomFactor, static_cast<float>(m_viewportWidth)),
        std::min(targetRect.Height * m_zoomFactor, static_cast<float>(m_viewportHeight))
    };

    winrt::Rect viewportRect{
        0.0f,
        0.0f,
        static_cast<float>(m_viewportWidth),
        static_cast<float>(m_viewportHeight),
    };

    if (targetZoomedHorizontalOffset != m_zoomedHorizontalOffset ||
        targetZoomedVerticalOffset != m_zoomedVerticalOffset)
    {
        com_ptr<ScrollOptions> options =
            winrt::make_self<ScrollOptions>(
                args.AnimationDesired() ? winrt::AnimationMode::Enabled : winrt::AnimationMode::Disabled,
                snapPointsMode);

        ChangeOffsetsPrivate(
            targetZoomedHorizontalOffset /*zoomedHorizontalOffset*/,
            targetZoomedVerticalOffset /*zoomedVerticalOffset*/,
            ScrollerViewKind::Absolute,
            *options,
            InteractionTrackerAsyncOperationTrigger::DirectViewChange,
            offsetsChangeId /*existingViewChangeId*/,
            nullptr /*viewChangeId*/);
    }
    else
    {
        // No offset change was triggered because the target offsets are the same as the current ones. Mark the operation as completed immediately.
        RaiseViewChangeCompleted(true /*isForScroll*/, ScrollerViewChangeResult::Completed, offsetsChangeId);
    }

    if (SharedHelpers::DoRectsIntersect(nextTargetRect, viewportRect))
    {
        // Next bring a portion of this Scroller into view.
        args.TargetRect(nextTargetRect);
        args.TargetElement(*this);
        args.HorizontalOffset(args.HorizontalOffset() - appliedOffsetX);
        args.VerticalOffset(args.VerticalOffset() - appliedOffsetY);
    }
    else
    {
        // This Scroller did not even partially bring the TargetRect into its viewport.
        // Mark the operation as handled since no portion of this Scroller needs to be brought into view.
        args.Handled(true);
    }
}

void Scroller::OnPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(m_scrollerVisualInteractionSource);

    if (m_horizontalScrollController && !m_horizontalScrollController.get().AreScrollerInteractionsAllowed())
    {
        return;
    }

    if (m_verticalScrollController && !m_verticalScrollController.get().AreScrollerInteractionsAllowed())
    {
        return;
    }

    const winrt::UIElement content = Content();
#ifdef USE_SCROLLMODE_AUTO
    const winrt::ScrollMode horizontalScrollMore = GetComputedScrollMode(ScrollerDimension::HorizontalScroll);
    const winrt::ScrollMode verticalScrollMore = GetComputedScrollMode(ScrollerDimension::VerticalScroll);
#else
    const winrt::ScrollMode horizontalScrollMore = HorizontalScrollMode();
    const winrt::ScrollMode verticalScrollMore = VerticalScrollMode();
#endif

    if (!content ||
        (horizontalScrollMore == winrt::ScrollMode::Disabled &&
         verticalScrollMore == winrt::ScrollMode::Disabled &&
         ZoomMode() == winrt::ZoomMode::Disabled))
    {
        return;
    }

    switch (args.Pointer().PointerDeviceType())
    {
        case winrt::Devices::Input::PointerDeviceType::Touch:
            if (IsInputKindIgnored(winrt::InputKind::Touch))
                return;
            break;
        case winrt::Devices::Input::PointerDeviceType::Pen:
            if (IsInputKindIgnored(winrt::InputKind::Pen))
                return;
            break;
        default:
            return;
    }

    // All UIElement instances between the touched one and the Scroller must include ManipulationModes.System in their
    // ManipulationMode property in order to trigger a manipulation. This allows to turn off touch interactions in particular.
    winrt::IInspectable source = args.OriginalSource();
    MUX_ASSERT(source);

    winrt::DependencyObject sourceAsDO = source.try_as<winrt::DependencyObject>();

    winrt::IUIElement thisAsUIElement = *this; // Need to have exactly the same interface as we're comparing below for object equality

    while (sourceAsDO)
    {
        winrt::IUIElement sourceAsUIE = sourceAsDO.try_as<winrt::IUIElement>();
        if (sourceAsUIE)
        {
            winrt::ManipulationModes mm = sourceAsUIE.ManipulationMode();

            if ((mm & winrt::ManipulationModes::System) == winrt::ManipulationModes::None)
            {
                return;
            }

            if (sourceAsUIE == thisAsUIElement)
            {
                break;
            }
        }

        sourceAsDO = winrt::VisualTreeHelper::GetParent(sourceAsDO);
    };

#ifdef _DEBUG
    DumpMinMaxPositions();
#endif // _DEBUG

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_STR, METH_NAME, this, L"TryRedirectForManipulation", TypeLogging::PointerPointToString(args.GetCurrentPoint(nullptr)).c_str());

    try
    {
        m_scrollerVisualInteractionSource.TryRedirectForManipulation(args.GetCurrentPoint(nullptr));
    }
    catch (const winrt::hresult_error& e)
    {
        // Swallowing Access Denied error because of InteractionTracker bug 17434718 which has been
        // causing crashes at least in RS3, RS4 and RS5.
        // TODO - Stop eating the error in future OS versions that include a fix for 17434718 if any.
        if (e.to_abi() != E_ACCESSDENIED)
        {
            throw;
        }
    }
}

void Scroller::OnXamlRootKeyDownOrUp(
    const winrt::IInspectable& /*sender*/,
    const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Control)
    {
        UpdateManipulationRedirectionMode();
    }
}

void Scroller::OnCoreWindowKeyDownOrUp(
    const winrt::CoreWindow& /*sender*/,
    const winrt::KeyEventArgs& args)
{
    if (args.VirtualKey() == winrt::VirtualKey::Control)
    {
        UpdateManipulationRedirectionMode();
    }
}

// Invoked by an IScrollController implementation when a call to InteractionTracker::TryRedirectForManipulation
// is required to track a finger.
void Scroller::OnScrollControllerInteractionRequested(
    const winrt::IScrollController& sender,
    const winrt::ScrollControllerInteractionRequestedEventArgs& args)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);

    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    if (!SharedHelpers::IsRS2OrHigher())
    {
        args.Handled(false);
        return;
    }

    if (args.Handled())
    {
        return;
    }

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = nullptr;

    if (sender == m_horizontalScrollController.get())
    {
        scrollControllerVisualInteractionSource = m_horizontalScrollControllerVisualInteractionSource;
    }
    else
    {
        scrollControllerVisualInteractionSource = m_verticalScrollControllerVisualInteractionSource;
    }

    if (scrollControllerVisualInteractionSource)
    {
        try
        {
            scrollControllerVisualInteractionSource.TryRedirectForManipulation(args.PointerPoint());
        }
        catch (const winrt::hresult_error& e)
        {
            // Swallowing Access Denied error because of InteractionTracker bug 17434718 which has been
            // causing crashes at least in RS3, RS4 and RS5.
            // TODO - Stop eating the error in future OS versions that include a fix for 17434718 if any.
            if (e.to_abi() == E_ACCESSDENIED)
            {
                // Do not set the Handled flag. The request is simply ignored.
                return;
            }
            else
            {
                throw;
            }
        }
        args.Handled(true);
    }
}

// Invoked by an IScrollController implementation when one or more of its characteristics has changed:
// InteractionVisual, InteractionVisualScrollOrientation or IsInteractionVisualRailEnabled.
void Scroller::OnScrollControllerInteractionInfoChanged(
    const winrt::IScrollController& sender,
    const winrt::IInspectable& /*args*/)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);

    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    if (!SharedHelpers::IsRS2OrHigher() || !m_interactionTracker)
    {
        return;
    }

    bool isFromHorizontalScrollController = sender == m_horizontalScrollController.get();

    winrt::CompositionPropertySet scrollControllerExpressionAnimationSources =
        isFromHorizontalScrollController ? m_horizontalScrollControllerExpressionAnimationSources : m_verticalScrollControllerExpressionAnimationSources;

    SetupScrollControllerVisualInterationSource(isFromHorizontalScrollController ? ScrollerDimension::HorizontalScroll : ScrollerDimension::VerticalScroll);

    if (isFromHorizontalScrollController)
    {
        if (scrollControllerExpressionAnimationSources != m_horizontalScrollControllerExpressionAnimationSources)
        {
            m_horizontalScrollController.get().SetExpressionAnimationSources(
                m_horizontalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
    else
    {
        if (scrollControllerExpressionAnimationSources != m_verticalScrollControllerExpressionAnimationSources)
        {
            m_verticalScrollController.get().SetExpressionAnimationSources(
                m_verticalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
}

// Invoked when a IScrollController::ScrollToRequested event is raised in order to perform the
// equivalent of a Scroller::ScrollTo operation.
void Scroller::OnScrollControllerScrollToRequested(
    const winrt::IScrollController& sender,
    const winrt::ScrollControllerScrollToRequestedEventArgs& args)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    bool isFromHorizontalScrollController = sender == m_horizontalScrollController.get();
    int32_t viewChangeId = -1;

    // Attempt to find an offset change request from an IScrollController with the same ScrollerViewKind,
    // the same ScrollOptions settings and same tick.
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationFromKinds(
        true /*isOperationTypeForOffsetsChange*/,
        static_cast<InteractionTrackerAsyncOperationTrigger>(static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) + static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest)),
        ScrollerViewKind::Absolute,
        args.Options());

    if (!interactionTrackerAsyncOperation)
    {
        ChangeOffsetsPrivate(
            isFromHorizontalScrollController ? args.Offset() : m_zoomedHorizontalOffset,
            isFromHorizontalScrollController ? m_zoomedVerticalOffset : args.Offset(),
            ScrollerViewKind::Absolute,
            args.Options(),
            isFromHorizontalScrollController ? InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest : InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest,
            -1 /*existingViewChangeId*/,
            &viewChangeId);
    }
    else
    {
        // Coalesce requests
        int32_t existingViewChangeId = interactionTrackerAsyncOperation->GetViewChangeId();
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();
        std::shared_ptr<OffsetsChange> offsetsChange = std::reinterpret_pointer_cast<OffsetsChange>(viewChangeBase);

        interactionTrackerAsyncOperation->SetIsScrollControllerRequest(isFromHorizontalScrollController);

        if (isFromHorizontalScrollController)
        {
            offsetsChange->ZoomedHorizontalOffset(args.Offset());
        }
        else
        {
            offsetsChange->ZoomedVerticalOffset(args.Offset());
        }

        viewChangeId = existingViewChangeId;
    }

    if (viewChangeId != -1)
    {
        args.Info(winrt::ScrollInfo{ viewChangeId });
    }
}

// Invoked when a IScrollController::ScrollByRequested event is raised in order to perform the
// equivalent of a Scroller::ScrollBy operation.
void Scroller::OnScrollControllerScrollByRequested(
    const winrt::IScrollController& sender,
    const winrt::ScrollControllerScrollByRequestedEventArgs& args)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    bool isFromHorizontalScrollController = sender == m_horizontalScrollController.get();
    int32_t viewChangeId = -1;

    // Attempt to find an offset change request from an IScrollController with the same ScrollerViewKind,
    // the same ScrollOptions settings and same tick.
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationFromKinds(
        true /*isOperationTypeForOffsetsChange*/,
        static_cast<InteractionTrackerAsyncOperationTrigger>(static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) + static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest)),
        ScrollerViewKind::RelativeToCurrentView,
        args.Options());

    if (!interactionTrackerAsyncOperation)
    {
        ChangeOffsetsPrivate(
            isFromHorizontalScrollController ? args.OffsetDelta() : 0.0 /*zoomedHorizontalOffset*/,
            isFromHorizontalScrollController ? 0.0 : args.OffsetDelta() /*zoomedVerticalOffset*/,
            ScrollerViewKind::RelativeToCurrentView,
            args.Options(),
            isFromHorizontalScrollController ? InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest : InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest,
            -1 /*existingViewChangeId*/,
            &viewChangeId);
    }
    else
    {
        // Coalesce requests
        int32_t existingViewChangeId = interactionTrackerAsyncOperation->GetViewChangeId();
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();
        std::shared_ptr<OffsetsChange> offsetsChange = std::reinterpret_pointer_cast<OffsetsChange>(viewChangeBase);

        interactionTrackerAsyncOperation->SetIsScrollControllerRequest(isFromHorizontalScrollController);

        if (isFromHorizontalScrollController)
        {
            offsetsChange->ZoomedHorizontalOffset(offsetsChange->ZoomedHorizontalOffset() + args.OffsetDelta());
        }
        else
        {
            offsetsChange->ZoomedVerticalOffset(offsetsChange->ZoomedVerticalOffset() + args.OffsetDelta());
        }

        viewChangeId = existingViewChangeId;
    }

    if (viewChangeId != -1)
    {
        args.Info(winrt::ScrollInfo{ viewChangeId });
    }
}

// Invoked when a IScrollController::ScrollFromRequested event is raised in order to perform the
// equivalent of a Scroller::ScrollFrom operation.
void Scroller::OnScrollControllerScrollFromRequested(
    const winrt::IScrollController& sender,
    const winrt::ScrollControllerScrollFromRequestedEventArgs& args)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);

    if (SharedHelpers::IsTH2OrLower())
    {
        throw winrt::hresult_error(E_NOTIMPL);
    }

    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    bool isFromHorizontalScrollController = sender == m_horizontalScrollController.get();
    int32_t viewChangeId = -1;
    winrt::IReference<float> horizontalInertiaDecayRate = nullptr;
    winrt::IReference<float> verticalInertiaDecayRate = nullptr;

    // Attempt to find an offset change with velocity request from an IScrollController and this same tick.
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationWithAdditionalVelocity(
        true /*isOperationTypeForOffsetsChange*/,
        static_cast<InteractionTrackerAsyncOperationTrigger>(static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) + static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest)));

    if (!interactionTrackerAsyncOperation)
    {
        winrt::IReference<winrt::float2> inertiaDecayRate = nullptr;
        winrt::float2 offsetsVelocity{};

        if (isFromHorizontalScrollController)
        {
            offsetsVelocity.x = args.OffsetVelocity();
            horizontalInertiaDecayRate = args.InertiaDecayRate();
        }
        else
        {
            offsetsVelocity.y = args.OffsetVelocity();
            verticalInertiaDecayRate = args.InertiaDecayRate();
        }

        if (horizontalInertiaDecayRate || verticalInertiaDecayRate)
        {
            winrt::IInspectable inertiaDecayRateAsInsp = nullptr;

            if (horizontalInertiaDecayRate)
            {
                inertiaDecayRateAsInsp = box_value(winrt::float2({ horizontalInertiaDecayRate.Value(), c_scrollerDefaultInertiaDecayRate }));
            }
            else
            {
                inertiaDecayRateAsInsp = box_value(winrt::float2({ c_scrollerDefaultInertiaDecayRate, verticalInertiaDecayRate.Value() }));
            }

            inertiaDecayRate = inertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();
        }

        ChangeOffsetsWithAdditionalVelocityPrivate(
            offsetsVelocity,
            inertiaDecayRate,
            isFromHorizontalScrollController ? InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest : InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest,
            &viewChangeId);
    }
    else
    {
        // Coalesce requests
        int32_t existingViewChangeId = interactionTrackerAsyncOperation->GetViewChangeId();
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();
        std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<OffsetsChangeWithAdditionalVelocity>(viewChangeBase);

        winrt::float2 offsetsVelocity = offsetsChangeWithAdditionalVelocity->OffsetsVelocity();
        winrt::IReference<winrt::float2> inertiaDecayRate = offsetsChangeWithAdditionalVelocity->InertiaDecayRate();

        interactionTrackerAsyncOperation->SetIsScrollControllerRequest(isFromHorizontalScrollController);

        if (isFromHorizontalScrollController)
        {
            offsetsVelocity.x = args.OffsetVelocity();
            horizontalInertiaDecayRate = args.InertiaDecayRate();

            if (!horizontalInertiaDecayRate)
            {
                if (inertiaDecayRate)
                {
                    if (inertiaDecayRate.Value().y == c_scrollerDefaultInertiaDecayRate)
                    {
                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(nullptr);
                    }
                    else
                    {
                        winrt::IInspectable newInertiaDecayRateAsInsp =
                            box_value(winrt::float2({ c_scrollerDefaultInertiaDecayRate, inertiaDecayRate.Value().y }));;
                        winrt::IReference<winrt::float2> newInertiaDecayRate =
                            newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
                    }
                }
            }
            else
            {
                winrt::IInspectable newInertiaDecayRateAsInsp = nullptr;

                if (!inertiaDecayRate)
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ horizontalInertiaDecayRate.Value(), c_scrollerDefaultInertiaDecayRate }));
                }
                else
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ horizontalInertiaDecayRate.Value(), inertiaDecayRate.Value().y }));
                }

                winrt::IReference<winrt::float2> newInertiaDecayRate = newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
            }
        }
        else
        {
            offsetsVelocity.y = args.OffsetVelocity();
            verticalInertiaDecayRate = args.InertiaDecayRate();

            if (!verticalInertiaDecayRate)
            {
                if (inertiaDecayRate)
                {
                    if (inertiaDecayRate.Value().x == c_scrollerDefaultInertiaDecayRate)
                    {
                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(nullptr);
                    }
                    else
                    {
                        winrt::IInspectable newInertiaDecayRateAsInsp =
                            box_value(winrt::float2({ inertiaDecayRate.Value().x, c_scrollerDefaultInertiaDecayRate }));;
                        winrt::IReference<winrt::float2> newInertiaDecayRate =
                            newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
                    }
                }
            }
            else
            {
                winrt::IInspectable newInertiaDecayRateAsInsp = nullptr;

                if (!inertiaDecayRate)
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ c_scrollerDefaultInertiaDecayRate, verticalInertiaDecayRate.Value() }));
                }
                else
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ inertiaDecayRate.Value().x, verticalInertiaDecayRate.Value() }));
                }

                winrt::IReference<winrt::float2> newInertiaDecayRate = newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
            }
        }

        offsetsChangeWithAdditionalVelocity->OffsetsVelocity(offsetsVelocity);

        viewChangeId = existingViewChangeId;
    }

    if (viewChangeId != -1)
    {
        args.Info(winrt::ScrollInfo{ viewChangeId });
    }
}

void Scroller::OnHorizontalSnapPointsVectorChanged(const winrt::IObservableVector<winrt::ScrollSnapPointBase>& sender, const winrt::IVectorChangedEventArgs args)
{
    SnapPointsVectorChangedHelper(sender, args, &m_sortedConsolidatedHorizontalSnapPoints, ScrollerDimension::HorizontalScroll);
}

void Scroller::OnVerticalSnapPointsVectorChanged(const winrt::IObservableVector<winrt::ScrollSnapPointBase>& sender, const winrt::IVectorChangedEventArgs args)
{
    SnapPointsVectorChangedHelper(sender, args, &m_sortedConsolidatedVerticalSnapPoints, ScrollerDimension::VerticalScroll);
}

void Scroller::OnZoomSnapPointsVectorChanged(const winrt::IObservableVector<winrt::ZoomSnapPointBase>& sender, const winrt::IVectorChangedEventArgs args)
{
    SnapPointsVectorChangedHelper(sender, args, &m_sortedConsolidatedZoomSnapPoints, ScrollerDimension::ZoomFactor);
}

template <typename T>
void Scroller::SnapPointsVectorChangedHelper(
    winrt::IObservableVector<T> const& snapPoints,
    winrt::IVectorChangedEventArgs const& args,
    std::set<T, winrtProjectionComparator>* snapPointsSet,
    ScrollerDimension dimension)
{
    MUX_ASSERT(!SharedHelpers::IsTH2OrLower());
    MUX_ASSERT(snapPoints);
    MUX_ASSERT(snapPointsSet);

    switch (args.CollectionChange())
    {
        case winrt::CollectionChange::Reset:
            snapPointsSet->clear();
            break;
        case winrt::CollectionChange::ItemInserted:
            SnapPointsVectorItemInsertedHelper(snapPoints.GetAt(args.Index()), snapPointsSet);
            break;
        case winrt::CollectionChange::ItemRemoved:
        case winrt::CollectionChange::ItemChanged:
            RegenerateSnapPointsSet(snapPoints, snapPointsSet);
            break;
        default:
            MUX_ASSERT(false);
    }

    SetupSnapPoints(snapPointsSet, dimension);
}

template <typename T>
void Scroller::SnapPointsVectorItemInsertedHelper(
    T changedItem,
    std::set<T, winrtProjectionComparator>* snapPointsSet)
{
    if (snapPointsSet->empty())
    {
        snapPointsSet->insert(changedItem);
        return;
    }

    winrt::SnapPointBase winrtChangedItem = safe_cast<winrt::SnapPointBase>(changedItem);
    auto lowerBound = snapPointsSet->lower_bound(changedItem);

    if (lowerBound != snapPointsSet->end())
    {
        winrt::SnapPointBase winrtSnapPointBase = safe_cast<winrt::SnapPointBase>(*lowerBound);
        SnapPointBase* lowerSnapPoint = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

        if (*lowerSnapPoint == winrt::get_self<SnapPointBase>(winrtChangedItem))
        {
            lowerSnapPoint->Combine(changedItem);
            return;
        }
        lowerBound++;
    }
    if (lowerBound != snapPointsSet->end())
    {
        winrt::SnapPointBase winrtSnapPointBase = safe_cast<winrt::SnapPointBase>(*lowerBound);
        SnapPointBase* upperSnapPoint = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

        if (*upperSnapPoint == winrt::get_self<SnapPointBase>(winrtChangedItem))
        {
            upperSnapPoint->Combine(changedItem);
            return;
        }
    }
    snapPointsSet->insert(changedItem);
}

template <typename T>
void Scroller::RegenerateSnapPointsSet(
    winrt::IObservableVector<T> const& userVector,
    std::set<T, winrtProjectionComparator>* internalSet)
{
    MUX_ASSERT(internalSet);

    internalSet->clear();
    for (T snapPoint : userVector)
    {
        SnapPointsVectorItemInsertedHelper(snapPoint, internalSet);
    }
}

void Scroller::UpdateContent(
    const winrt::UIElement& oldContent,
    const winrt::UIElement& newContent)
{
    const bool useTranslationProperty = IsVisualTranslationPropertyAvailable();

    auto children = Children();
    children.Clear();

    UnhookContentPropertyChanged(oldContent);

    if (newContent)
    {
        children.Append(newContent);

        if (!SharedHelpers::IsTH2OrLower())
        {
            if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
            {
                UpdatePositionBoundaries(newContent);
            }
            else if (m_interactionTracker)
            {
                EnsurePositionBoundariesExpressionAnimations();
                SetupPositionBoundariesExpressionAnimations(newContent);
            }

            if ((m_transformMatrixTranslateXExpressionAnimation && m_transformMatrixTranslateYExpressionAnimation && m_transformMatrixZoomFactorExpressionAnimation && !useTranslationProperty) ||
                (m_translationExpressionAnimation && m_zoomFactorExpressionAnimation && useTranslationProperty))
            {
                UpdateTransformSource(oldContent, newContent);
            }
            else if (m_interactionTracker)
            {
                EnsureTransformExpressionAnimations();
                SetupTransformExpressionAnimations(newContent);
            }

            HookContentPropertyChanged(newContent);
        }
    }
    else
    {
        if (m_contentLayoutOffsetX != 0.0f)
        {
            m_contentLayoutOffsetX = 0.0f;
            OnContentLayoutOffsetChanged(ScrollerDimension::HorizontalScroll);
        }

        if (m_contentLayoutOffsetY != 0.0f)
        {
            m_contentLayoutOffsetY = 0.0f;
            OnContentLayoutOffsetChanged(ScrollerDimension::VerticalScroll);
        }

        if (!m_interactionTracker || (m_zoomedHorizontalOffset == 0.0 && m_zoomedVerticalOffset == 0.0))
        {
            // Complete all active or delayed operations when there is no InteractionTracker, when the old content
            // was already at offsets (0,0). The ScrollToOffsets request below will result in their completion otherwise.
            CompleteInteractionTrackerOperations(
                -1 /*requestId*/,
                ScrollerViewChangeResult::Interrupted /*operationResult*/,
                ScrollerViewChangeResult::Ignored     /*unused priorNonAnimatedOperationsResult*/,
                ScrollerViewChangeResult::Ignored     /*unused priorAnimatedOperationsResult*/,
                true  /*completeOperation*/,
                false /*completePriorNonAnimatedOperations*/,
                false /*completePriorAnimatedOperations*/);
        }

        if (m_interactionTracker)
        {
            if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
            {
                UpdatePositionBoundaries(nullptr);
            }
            if ((m_transformMatrixTranslateXExpressionAnimation && m_transformMatrixTranslateYExpressionAnimation && m_transformMatrixZoomFactorExpressionAnimation && !useTranslationProperty) ||
                (m_translationExpressionAnimation && m_zoomFactorExpressionAnimation && useTranslationProperty))
            {
                StopTransformExpressionAnimations(oldContent);
            }
            ScrollToOffsets(0.0 /*zoomedHorizontalOffset*/, 0.0 /*zoomedVerticalOffset*/);
        }
    }
}

void Scroller::UpdatePositionBoundaries(
    const winrt::UIElement& content)
{
    MUX_ASSERT(m_minPositionExpressionAnimation);
    MUX_ASSERT(m_maxPositionExpressionAnimation);
    MUX_ASSERT(m_interactionTracker);

    if (!content)
    {
        const winrt::float3 boundaryPosition(0.0f);

        m_interactionTracker.MinPosition(boundaryPosition);
        m_interactionTracker.MaxPosition(boundaryPosition);
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentSizeX", m_unzoomedExtentWidth);
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentSizeY", m_unzoomedExtentHeight);
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"contentLayoutOffsetX", m_contentLayoutOffsetX);
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"contentLayoutOffsetY", m_contentLayoutOffsetY);

        m_minPositionExpressionAnimation.SetScalarParameter(L"contentSizeX", static_cast<float>(m_unzoomedExtentWidth));
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentSizeX", static_cast<float>(m_unzoomedExtentWidth));
        m_minPositionExpressionAnimation.SetScalarParameter(L"contentSizeY", static_cast<float>(m_unzoomedExtentHeight));
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentSizeY", static_cast<float>(m_unzoomedExtentHeight));

        m_minPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetX", m_contentLayoutOffsetX);
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetX", m_contentLayoutOffsetX);
        m_minPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetY", m_contentLayoutOffsetY);
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetY", m_contentLayoutOffsetY);

        m_interactionTracker.StartAnimation(L"MinPosition", m_minPositionExpressionAnimation);
        m_interactionTracker.StartAnimation(L"MaxPosition", m_maxPositionExpressionAnimation);
    }

#ifdef _DEBUG
    DumpMinMaxPositions();
#endif // _DEBUG
}

void Scroller::UpdateTransformSource(
    const winrt::UIElement& oldContent,
    const winrt::UIElement& newContent)
{
    MUX_ASSERT((m_transformMatrixTranslateXExpressionAnimation && m_transformMatrixTranslateYExpressionAnimation && m_transformMatrixZoomFactorExpressionAnimation && !IsVisualTranslationPropertyAvailable()) ||
        (m_translationExpressionAnimation && m_zoomFactorExpressionAnimation && IsVisualTranslationPropertyAvailable()));
    MUX_ASSERT(m_interactionTracker);

    StopTransformExpressionAnimations(oldContent);
    StartTransformExpressionAnimations(newContent);
}

void Scroller::UpdateState(
    const winrt::InteractionState& state)
{
    if (state != m_state)
    {
        m_state = state;
        RaiseStateChanged();
    }
}

void Scroller::UpdateExpressionAnimationSources()
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(m_expressionAnimationSources);

    m_expressionAnimationSources.InsertVector2(s_extentSourcePropertyName, { static_cast<float>(m_unzoomedExtentWidth), static_cast<float>(m_unzoomedExtentHeight) });
    m_expressionAnimationSources.InsertVector2(s_viewportSourcePropertyName, { static_cast<float>(m_viewportWidth), static_cast<float>(m_viewportHeight) });
}

void Scroller::UpdateUnzoomedExtentAndViewport(
    double unzoomedExtentWidth, double unzoomedExtentHeight,
    double viewportWidth, double viewportHeight)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"unzoomedExtentWidth", unzoomedExtentWidth);
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"unzoomedExtentHeight", unzoomedExtentHeight);
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"viewportWidth", viewportWidth);
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"viewportHeight", viewportHeight);

    const winrt::UIElement content = Content();
    const winrt::UIElement thisAsUIE = *this;
    const double oldUnzoomedExtentWidth = m_unzoomedExtentWidth;
    const double oldUnzoomedExtentHeight = m_unzoomedExtentHeight;
    const double oldViewportWidth = m_viewportWidth;
    const double oldViewportHeight = m_viewportHeight;

    MUX_ASSERT(!isinf(unzoomedExtentWidth));
    MUX_ASSERT(!isnan(unzoomedExtentWidth));
    MUX_ASSERT(!isinf(unzoomedExtentHeight));
    MUX_ASSERT(!isnan(unzoomedExtentHeight));

    MUX_ASSERT(!isinf(viewportWidth));
    MUX_ASSERT(!isnan(viewportWidth));
    MUX_ASSERT(!isinf(viewportHeight));
    MUX_ASSERT(!isnan(viewportHeight));

    MUX_ASSERT(unzoomedExtentWidth >= 0.0);
    MUX_ASSERT(unzoomedExtentHeight >= 0.0);
    MUX_ASSERT(!(!content && unzoomedExtentWidth != 0.0));
    MUX_ASSERT(!(!content && unzoomedExtentHeight != 0.0));

    const bool horizontalExtentChanged = oldUnzoomedExtentWidth != unzoomedExtentWidth;
    const bool verticalExtentChanged = oldUnzoomedExtentHeight != unzoomedExtentHeight;
    const bool extentChanged = horizontalExtentChanged || verticalExtentChanged;

    const bool horizontalViewportChanged = oldViewportWidth != viewportWidth;
    const bool verticalViewportChanged = oldViewportHeight != viewportHeight;
    const bool viewportChanged = horizontalViewportChanged || verticalViewportChanged;

    m_unzoomedExtentWidth = unzoomedExtentWidth;
    m_unzoomedExtentHeight = unzoomedExtentHeight;

    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;

    if (m_expressionAnimationSources)
    {
        UpdateExpressionAnimationSources();
    }

    if (extentChanged && content)
    {
        OnContentSizeChanged(content);
    }

    if (extentChanged || viewportChanged)
    {
        UpdateScrollAutomationPatternProperties();
    }

    if (horizontalExtentChanged || horizontalViewportChanged)
    {
#ifdef USE_SCROLLMODE_AUTO
        // Updating the horizontal scroll mode because GetComputedScrollMode is function of the scrollable width.
        UpdateVisualInteractionSourceMode(ScrollerDimension::HorizontalScroll);
#endif
        UpdateScrollControllerValues(ScrollerDimension::HorizontalScroll);
    }

    if (verticalExtentChanged || verticalViewportChanged)
    {
#ifdef USE_SCROLLMODE_AUTO
        // Updating the vertical scroll mode because GetComputedScrollMode is function of the scrollable height.
        UpdateVisualInteractionSourceMode(ScrollerDimension::VerticalScroll);
#endif
        UpdateScrollControllerValues(ScrollerDimension::VerticalScroll);
    }

    if (extentChanged)
    {
        RaiseExtentChanged();
    }
}

// Raise automation peer property change events
void Scroller::UpdateScrollAutomationPatternProperties()
{
    if (winrt::AutomationPeer automationPeer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        winrt::ScrollerAutomationPeer scrollerAutomationPeer = automationPeer.try_as<winrt::ScrollerAutomationPeer>();
        if (scrollerAutomationPeer)
        {
            winrt::get_self<ScrollerAutomationPeer>(scrollerAutomationPeer)->UpdateScrollPatternProperties();
        }
    }
}

void Scroller::UpdateOffset(ScrollerDimension dimension, double zoomedOffset)
{
    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        if (m_zoomedHorizontalOffset != zoomedOffset)
        {
            SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"zoomedHorizontalOffset", zoomedOffset);
            m_zoomedHorizontalOffset = zoomedOffset;
        }
    }
    else
    {
        MUX_ASSERT(dimension == ScrollerDimension::VerticalScroll);
        if (m_zoomedVerticalOffset != zoomedOffset)
        {
            SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"zoomedVerticalOffset", zoomedOffset);
            m_zoomedVerticalOffset = zoomedOffset;
        }
    }
}

void Scroller::UpdateScrollControllerInteractionsAllowed(ScrollerDimension dimension)
{
    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        if (m_horizontalScrollController)
        {
            m_horizontalScrollController.get().SetScrollMode(HorizontalScrollMode());
        }
    }
    else
    {
        MUX_ASSERT(dimension == ScrollerDimension::VerticalScroll);

        if (m_verticalScrollController)
        {
            m_verticalScrollController.get().SetScrollMode(VerticalScrollMode());
        }
    }
}

void Scroller::UpdateScrollControllerValues(ScrollerDimension dimension)
{
    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        if (m_horizontalScrollController)
        {
            m_horizontalScrollController.get().SetValues(
                0.0 /*minOffset*/,
                ScrollableWidth() /*maxOffset*/,
                m_zoomedHorizontalOffset /*offset*/,
                ViewportWidth() /*viewport*/);
        }
    }
    else
    {
        MUX_ASSERT(dimension == ScrollerDimension::VerticalScroll);

        if (m_verticalScrollController)
        {
            m_verticalScrollController.get().SetValues(
                0.0 /*minOffset*/,
                ScrollableHeight() /*maxOffset*/,
                m_zoomedVerticalOffset /*offset*/,
                ViewportHeight() /*viewport*/);
        }
    }
}

void Scroller::UpdateVisualInteractionSourceMode(ScrollerDimension dimension)
{
#ifdef USE_SCROLLMODE_AUTO
    const winrt::ScrollMode scrollMode = GetComputedScrollMode(dimension);
#else
    const winrt::ScrollMode scrollMode = dimension == ScrollerDimension::HorizontalScroll ? HorizontalScrollMode() : VerticalScrollMode();
#endif

    if (m_scrollerVisualInteractionSource)
    {
        SetupVisualInteractionSourceMode(
            m_scrollerVisualInteractionSource,
            dimension,
            scrollMode);

#ifdef IsMouseWheelScrollDisabled
        if (SharedHelpers::IsRS5OrHigher())
        {
            SetupVisualInteractionSourcePointerWheelConfig(
                m_scrollerVisualInteractionSource,
                dimension,
                GetComputedMouseWheelScrollMode(dimension));
        }
#endif
    }

    UpdateScrollControllerInteractionsAllowed(dimension);
}

void Scroller::UpdateManipulationRedirectionMode()
{
    if (m_scrollerVisualInteractionSource)
    {
        winrt::InputKind ignoredInputKind = IgnoredInputKind();

        if (!IsInputKindIgnored(winrt::InputKind::MouseWheel))
        {
            bool suppressMouseWheel = true;

            if (SharedHelpers::IsRS4OrHigher())
            {
                // Suppressing InteractionTracker's mouse wheel support when a Control key is down so the Scroller
                // can receive mouse wheel messages to zoom in and out.
                // Starting with RS5, the InteractionTracker supports built-in zoom in and out with the Control key and mouse wheel, 
                // so no suppression is required anymore.
                if (Scroller::IsInteractionTrackerMouseWheelZoomingEnabled())
                {
                    suppressMouseWheel = false;
                }
                else
                {
                    winrt::CoreVirtualKeyStates ctrlState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Control);
                    suppressMouseWheel = (ctrlState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
                }
            }

            if (suppressMouseWheel)
            {
                ignoredInputKind |= winrt::InputKind::MouseWheel;
            }
        }

        SetupVisualInteractionSourceRedirectionMode(
            m_scrollerVisualInteractionSource,
            ignoredInputKind);
    }
}

void Scroller::UpdateKeyEvents()
{
    MUX_ASSERT(m_scrollerVisualInteractionSource);

    if (SharedHelpers::IsRS4OrHigher() && !Scroller::IsInteractionTrackerMouseWheelZoomingEnabled())
    {
        if (!IsInputKindIgnored(winrt::InputKind::MouseWheel))
        {
            SetKeyEvents();
        }
        else
        {
            ResetKeyEvents();
        }
    }
}

void Scroller::OnContentSizeChanged(const winrt::UIElement& content)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
    {
        UpdatePositionBoundaries(content);
    }

    const bool useTranslationProperty = IsVisualTranslationPropertyAvailable();

    if (m_interactionTracker &&
        ((!useTranslationProperty && m_transformMatrixTranslateXExpressionAnimation && m_transformMatrixTranslateYExpressionAnimation && m_transformMatrixZoomFactorExpressionAnimation) ||
        (useTranslationProperty && m_translationExpressionAnimation && m_zoomFactorExpressionAnimation)))
    {
        SetupTransformExpressionAnimations(content);
    }
}

void Scroller::OnViewChanged(bool horizontalOffsetChanged, bool verticalOffsetChanged)
{
    //SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, horizontalOffsetChanged, verticalOffsetChanged);
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL_FLT, METH_NAME, this, m_zoomedHorizontalOffset, m_zoomedVerticalOffset, m_zoomFactor);

    if (horizontalOffsetChanged)
    {
        UpdateScrollControllerValues(ScrollerDimension::HorizontalScroll);
    }

    if (verticalOffsetChanged)
    {
        UpdateScrollControllerValues(ScrollerDimension::VerticalScroll);
    }

    UpdateScrollAutomationPatternProperties();

    RaiseViewChanged();
}

void Scroller::OnContentLayoutOffsetChanged(ScrollerDimension dimension)
{
    MUX_ASSERT(dimension == ScrollerDimension::HorizontalScroll || dimension == ScrollerDimension::VerticalScroll);

    if (dimension == ScrollerDimension::HorizontalScroll)
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"Horizontal", m_contentLayoutOffsetX);
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"Vertical", m_contentLayoutOffsetY);
    }

    com_ptr<ScrollerTestHooks> globalTestHooks = ScrollerTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        if (dimension == ScrollerDimension::HorizontalScroll)
        {
            globalTestHooks->NotifyContentLayoutOffsetXChanged(*this);
        }
        else
        {
            globalTestHooks->NotifyContentLayoutOffsetYChanged(*this);
        }
    }

    if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
    {
        const winrt::UIElement content = Content();

        if (content)
        {
            UpdatePositionBoundaries(content);
        }
    }

    if (m_expressionAnimationSources)
    {
        m_expressionAnimationSources.InsertVector2(s_offsetSourcePropertyName, { m_contentLayoutOffsetX, m_contentLayoutOffsetY });
    }

    if (SharedHelpers::IsRS2OrHigher() && m_scrollerVisualInteractionSource)
    {
        SetupVisualInteractionSourceCenterPointModifier(
            m_scrollerVisualInteractionSource,
            dimension);
    }
}

void Scroller::ChangeOffsetsPrivate(
    double zoomedHorizontalOffset,
    double zoomedVerticalOffset,
    ScrollerViewKind offsetsKind,
    winrt::ScrollOptions const& options,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    int32_t existingViewChangeId,
    _Out_opt_ int32_t* viewChangeId)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        zoomedHorizontalOffset,
        zoomedVerticalOffset,
        TypeLogging::ScrollOptionsToString(options).c_str());
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str(),
        TypeLogging::ScrollerViewKindToString(offsetsKind).c_str());

    if (viewChangeId)
    {
        *viewChangeId = -1;
    }

    winrt::AnimationMode animationMode = options ? options.AnimationMode() : ScrollOptions::s_defaultAnimationMode;
    winrt::SnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollOptions::s_defaultSnapPointsMode;
    InteractionTrackerAsyncOperationType operationType;

    switch (animationMode)
    {
    case winrt::AnimationMode::Disabled:
    {
        switch (offsetsKind)
        {
        case ScrollerViewKind::Absolute:
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
        case ScrollerViewKind::RelativeToEndOfInertiaView:
#endif
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdatePosition;
            break;
        }
        case ScrollerViewKind::RelativeToCurrentView:
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdatePositionBy;
            break;
        }
        }
        break;
    }
    case winrt::AnimationMode::Enabled:
    {
        switch (offsetsKind)
        {
        case ScrollerViewKind::Absolute:
        case ScrollerViewKind::RelativeToCurrentView:
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
        case ScrollerViewKind::RelativeToEndOfInertiaView:
#endif
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation;
            break;
        }
        }
        break;
    }
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    // When the Scroller is not loaded or not set up yet, delay the offsets change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    // Set to True when workaround for RS5 InteractionTracker bug 18827625 was applied (i.e. on-going TryUpdatePositionWithAnimation operation
    // is interrupted with TryUpdatePositionBy operation).
    bool offsetsChangeWithAnimationInterrupted = false;

    com_ptr<ScrollOptions> optionsClone{ nullptr };

    // Clone the options for this request if needed. The clone or original options will be used if the operation ever gets processed.
    const bool isScrollControllerRequest =
        static_cast<char>(operationTrigger) &
        (static_cast<char>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) |
            static_cast<char>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest));

    if (options && !isScrollControllerRequest)
    {
        // Options are cloned so that they can be modified by the caller after this offsets change call without affecting the outcome of the operation.
        optionsClone = winrt::make_self<ScrollOptions>(
            animationMode,
            snapPointsMode);
    }

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();

        if (animationMode == winrt::AnimationMode::Enabled)
        {
            // Workaround for RS5 InteractionTracker bug 18827625: Interrupt on-going TryUpdatePositionWithAnimation
            // operation before launching new one.
            offsetsChangeWithAnimationInterrupted = InterruptViewChangeWithAnimation(InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation);
        }
    }

    std::shared_ptr<ViewChange> offsetsChange = 
        std::make_shared<OffsetsChange>(
            zoomedHorizontalOffset,
            zoomedVerticalOffset,
            offsetsKind,
            optionsClone ? winrt::IInspectable{ *optionsClone } : options); // NOTE: Using explicit cast to winrt::IInspectable to work around 17532876

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            operationType,
            operationTrigger,
            delayOperation,
            offsetsChange));

    if (offsetsChangeWithAnimationInterrupted)
    {
        // Adding an extra tick to the default countdown to allow the InteractionTracker to reach and remain in the Idle state
        // long enough to avoid its RS5 bug 18827625.
        interactionTrackerAsyncOperation->SetTicksCountdown(interactionTrackerAsyncOperation->GetTicksCountdown() + 1);
    }
    else if (operationTrigger != InteractionTrackerAsyncOperationTrigger::DirectViewChange)
    {
        // User-triggered operations are processed as quickly as possible by minimizing their TicksCountDown
        int ticksCountdown = GetInteractionTrackerOperationsTicksCountdownForTrigger(InteractionTrackerAsyncOperationTrigger::DirectViewChange);

        interactionTrackerAsyncOperation->SetTicksCountdown(std::max(1, ticksCountdown));
    }

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    if (viewChangeId)
    {
        if (existingViewChangeId != -1)
        {
            interactionTrackerAsyncOperation->SetViewChangeId(existingViewChangeId);
            *viewChangeId = existingViewChangeId;
        }
        else
        {
            m_latestViewChangeId = GetNextViewChangeId();
            interactionTrackerAsyncOperation->SetViewChangeId(m_latestViewChangeId);
            *viewChangeId = m_latestViewChangeId;
        }
    }
    else if (existingViewChangeId != -1)
    {
        interactionTrackerAsyncOperation->SetViewChangeId(existingViewChangeId);
    }
}

void Scroller::ChangeOffsetsWithAdditionalVelocityPrivate(
    winrt::float2 offsetsVelocity,
    winrt::IReference<winrt::float2> inertiaDecayRate,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    _Out_opt_ int32_t* viewChangeId)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(),
        TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str(),
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str());

    if (viewChangeId)
    {
        *viewChangeId = -1;
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    // When the Scroller is not loaded or not set up yet, delay the offsets change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    std::shared_ptr<ViewChangeBase> offsetsChangeWithAdditionalVelocity =
        std::make_shared<OffsetsChangeWithAdditionalVelocity>(
            offsetsVelocity, inertiaDecayRate);

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();
    }

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity,
            operationTrigger,
            delayOperation,
            offsetsChangeWithAdditionalVelocity));

    if (operationTrigger != InteractionTrackerAsyncOperationTrigger::DirectViewChange)
    {
        // User-triggered operations are processed as quickly as possible by minimizing their TicksCountDown
        int ticksCountdown = GetInteractionTrackerOperationsTicksCountdownForTrigger(InteractionTrackerAsyncOperationTrigger::DirectViewChange);

        interactionTrackerAsyncOperation->SetTicksCountdown(std::max(1, ticksCountdown));
    }

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    if (viewChangeId)
    {
        m_latestViewChangeId = GetNextViewChangeId();
        interactionTrackerAsyncOperation->SetViewChangeId(m_latestViewChangeId);
        *viewChangeId = m_latestViewChangeId;
    }
}

void Scroller::ChangeZoomFactorPrivate(
    float zoomFactor,
    winrt::IReference<winrt::float2> centerPoint,
    ScrollerViewKind zoomFactorKind,
    winrt::ZoomOptions const& options,
    _Out_opt_ int32_t* viewChangeId)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        zoomFactor);
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,        
        TypeLogging::ScrollerViewKindToString(zoomFactorKind).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str());

    if (viewChangeId)
    {
        *viewChangeId = -1;
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    winrt::AnimationMode animationMode = options ? options.AnimationMode() : ZoomOptions::s_defaultAnimationMode;
    winrt::SnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ZoomOptions::s_defaultSnapPointsMode;
    InteractionTrackerAsyncOperationType operationType;

    switch (animationMode)
    {
        case winrt::AnimationMode::Disabled:
        {
            switch (zoomFactorKind)
            {
                case ScrollerViewKind::Absolute:
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
                case ScrollerViewKind::RelativeToEndOfInertiaView:
#endif
                case ScrollerViewKind::RelativeToCurrentView:
                {
                    operationType = InteractionTrackerAsyncOperationType::TryUpdateScale;
                    break;
                }
            }
            break;
        }
        case winrt::AnimationMode::Enabled:
        {
            switch (zoomFactorKind)
            {
                case ScrollerViewKind::Absolute:
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
                case ScrollerViewKind::RelativeToEndOfInertiaView:
#endif
                case ScrollerViewKind::RelativeToCurrentView:
                {
                    operationType = InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation;
                    break;
                }
            }
            break;
        }
    }

    // When the Scroller is not loaded or not set up yet (delayOperation==True), delay the zoomFactor change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    // Set to True when workaround for RS5 InteractionTracker bug 18827625 was applied (i.e. on-going TryUpdateScaleWithAnimation operation
    // is interrupted with TryUpdateScale operation).
    bool scaleChangeWithAnimationInterrupted = false;

    com_ptr<ZoomOptions> optionsClone{ nullptr };

    // Clone the original options if any. The clone will be used if the operation ever gets processed.
    if (options)
    {
        // Options are cloned so that they can be modified by the caller after this zoom factor change call without affecting the outcome of the operation.
        optionsClone = winrt::make_self<ZoomOptions>(
            animationMode,
            snapPointsMode);
    }

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();

        if (animationMode == winrt::AnimationMode::Enabled)
        {
            // Workaround for RS5 InteractionTracker bug 18827625: Interrupt on-going TryUpdateScaleWithAnimation
            // operation before launching new one.
            scaleChangeWithAnimationInterrupted = InterruptViewChangeWithAnimation(InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation);
        }
    }

    std::shared_ptr<ViewChange> zoomFactorChange =
        std::make_shared<ZoomFactorChange>(
            zoomFactor,
            centerPoint,
            zoomFactorKind, 
            optionsClone ? winrt::IInspectable{ *optionsClone } : options); // NOTE: Using explicit cast to winrt::IInspectable to work around 17532876

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            operationType,
            InteractionTrackerAsyncOperationTrigger::DirectViewChange,
            delayOperation,
            zoomFactorChange));

    if (scaleChangeWithAnimationInterrupted)
    {
        // Adding an extra tick to the default countdown to allow the InteractionTracker to reach and remain in the Idle state
        // long enough to avoid its RS5 bug 18827625.
        interactionTrackerAsyncOperation->SetTicksCountdown(interactionTrackerAsyncOperation->GetTicksCountdown() + 1);
    }

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    if (viewChangeId)
    {
        m_latestViewChangeId = GetNextViewChangeId();
        interactionTrackerAsyncOperation->SetViewChangeId(m_latestViewChangeId);
        *viewChangeId = m_latestViewChangeId;
    }
}

void Scroller::ChangeZoomFactorWithAdditionalVelocityPrivate(
    float zoomFactorVelocity,
    winrt::IReference<winrt::float2> centerPoint,
    winrt::IReference<float> inertiaDecayRate,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    _Out_opt_ int32_t* viewChangeId)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(),
        zoomFactorVelocity);
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str());

    if (viewChangeId)
    {
        *viewChangeId = -1;
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    // When the Scroller is not loaded or not set up yet (delayOperation==True), delay the zoom factor change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    std::shared_ptr<ViewChangeBase> zoomFactorChangeWithAdditionalVelocity =
        std::make_shared<ZoomFactorChangeWithAdditionalVelocity>(
            zoomFactorVelocity, centerPoint, inertiaDecayRate);

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();
    }

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity,
            operationTrigger,
            delayOperation,
            zoomFactorChangeWithAdditionalVelocity));

    if (operationTrigger != InteractionTrackerAsyncOperationTrigger::DirectViewChange)
    {
        // User-triggered operations are processed as quickly as possible by minimizing their TicksCountDown
        int ticksCountdown = GetInteractionTrackerOperationsTicksCountdownForTrigger(InteractionTrackerAsyncOperationTrigger::DirectViewChange);

        interactionTrackerAsyncOperation->SetTicksCountdown(std::max(1, ticksCountdown));
    }

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    if (viewChangeId)
    {
        m_latestViewChangeId = GetNextViewChangeId();
        interactionTrackerAsyncOperation->SetViewChangeId(m_latestViewChangeId);
        *viewChangeId = m_latestViewChangeId;
    }
}

void Scroller::ProcessDequeuedViewChange(std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, interactionTrackerAsyncOperation);

    MUX_ASSERT(IsLoadedAndSetUp());
    MUX_ASSERT(!interactionTrackerAsyncOperation->IsQueued());

    std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();

    MUX_ASSERT(viewChangeBase);

    switch (interactionTrackerAsyncOperation->GetOperationType())
    {
        case InteractionTrackerAsyncOperationType::TryUpdatePosition:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionBy:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation:
        {
            std::shared_ptr<OffsetsChange> offsetsChange = std::reinterpret_pointer_cast<OffsetsChange>(viewChangeBase);

            ProcessOffsetsChange(
                interactionTrackerAsyncOperation->GetOperationTrigger() /*operationTrigger*/,
                offsetsChange,
                interactionTrackerAsyncOperation->GetViewChangeId() /*offsetsChangeId*/,
                true /*isForAsyncOperation*/);
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
        {
            std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<OffsetsChangeWithAdditionalVelocity>(viewChangeBase);

            ProcessOffsetsChange(offsetsChangeWithAdditionalVelocity);
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdateScale:
        case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation:
        {
            std::shared_ptr<ZoomFactorChange> zoomFactorChange = std::reinterpret_pointer_cast<ZoomFactorChange>(viewChangeBase);

            ProcessZoomFactorChange(
                zoomFactorChange,
                interactionTrackerAsyncOperation->GetViewChangeId() /*zoomFactorChangeId*/);
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity:
        {
            std::shared_ptr<ZoomFactorChangeWithAdditionalVelocity> zoomFactorChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<ZoomFactorChangeWithAdditionalVelocity>(viewChangeBase);

            ProcessZoomFactorChange(zoomFactorChangeWithAdditionalVelocity);
            break;
        }
        default:
        {
            MUX_ASSERT(false);
        }
    }
    interactionTrackerAsyncOperation->SetRequestId(m_latestInteractionTrackerRequest);
}

// Launches an InteractionTracker request to change the offsets.
void Scroller::ProcessOffsetsChange(
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    std::shared_ptr<OffsetsChange> offsetsChange,
    int32_t offsetsChangeId,
    bool isForAsyncOperation)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(offsetsChange);

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR_INT_INT, METH_NAME, this,
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str(),
        TypeLogging::ScrollerViewKindToString(offsetsChange->ViewKind()).c_str(),
        offsetsChangeId,
        isForAsyncOperation);

    double zoomedHorizontalOffset = offsetsChange->ZoomedHorizontalOffset();
    double zoomedVerticalOffset = offsetsChange->ZoomedVerticalOffset();
    winrt::ScrollOptions options = offsetsChange->Options().try_as<winrt::ScrollOptions>();

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        zoomedHorizontalOffset,
        zoomedVerticalOffset,
        TypeLogging::ScrollOptionsToString(options).c_str());

    winrt::AnimationMode animationMode = options ? options.AnimationMode() : ScrollOptions::s_defaultAnimationMode;
    winrt::SnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollOptions::s_defaultSnapPointsMode;

    switch (offsetsChange->ViewKind())
    {
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
        case ScrollerViewKind::RelativeToEndOfInertiaView:
        {
            const winrt::float2 endOfInertiaPosition = ComputeEndOfInertiaPosition();
            zoomedHorizontalOffset += endOfInertiaPosition.x;
            zoomedVerticalOffset += endOfInertiaPosition.y;
            break;
        }
#endif
        case ScrollerViewKind::RelativeToCurrentView:
        {
            if (snapPointsMode == winrt::SnapPointsMode::Default || animationMode == winrt::AnimationMode::Enabled)
            {
                zoomedHorizontalOffset += m_zoomedHorizontalOffset;
                zoomedVerticalOffset += m_zoomedVerticalOffset;
            }
            break;
        }
    }

    if (snapPointsMode == winrt::SnapPointsMode::Default)
    {
        zoomedHorizontalOffset = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(zoomedHorizontalOffset, m_sortedConsolidatedHorizontalSnapPoints);
        zoomedVerticalOffset = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(zoomedVerticalOffset, m_sortedConsolidatedVerticalSnapPoints);
    }

    switch (animationMode)
    {
        case winrt::AnimationMode::Disabled:
        {
            if (offsetsChange->ViewKind() == ScrollerViewKind::RelativeToCurrentView && snapPointsMode == winrt::SnapPointsMode::Ignore)
            {
                SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_STR, METH_NAME,
                    this,
                    L"TryUpdatePositionBy",
                    TypeLogging::Float2ToString(winrt::float2(static_cast<float>(zoomedHorizontalOffset), static_cast<float>(zoomedVerticalOffset))).c_str());

                m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePositionBy(
                    winrt::float3(static_cast<float>(zoomedHorizontalOffset), static_cast<float>(zoomedVerticalOffset), 0.0f));
                m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePositionBy;
            }
            else
            {
                winrt::float2 targetPosition = ComputePositionFromOffsets(zoomedHorizontalOffset, zoomedVerticalOffset);

                SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_STR, METH_NAME, this,
                    L"TryUpdatePosition", TypeLogging::Float2ToString(targetPosition).c_str());

                m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePosition(
                    winrt::float3(targetPosition, 0.0f));
                m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePosition;
            }

            if (isForAsyncOperation)
            {
                HookCompositionTargetRendering();
            }
            break;
        }
        case winrt::AnimationMode::Enabled:
        {
            SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"TryUpdatePositionWithAnimation");

            m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePositionWithAnimation(
                GetPositionAnimation(
                    zoomedHorizontalOffset,
                    zoomedVerticalOffset,
                    operationTrigger,
                    offsetsChangeId));
            m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation;
            break;
        }
    }
}

// Launches an InteractionTracker request to change the offsets with an additional velocity and optional scroll inertia decay rate.
void Scroller::ProcessOffsetsChange(
    std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(offsetsChangeWithAdditionalVelocity);

    winrt::float2 offsetsVelocity = offsetsChangeWithAdditionalVelocity->OffsetsVelocity();
    winrt::IReference<winrt::float2> inertiaDecayRate = offsetsChangeWithAdditionalVelocity->InertiaDecayRate();

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    if (inertiaDecayRate)
    {
        float horizontalInertiaDecayRate = std::clamp(inertiaDecayRate.Value().x, 0.0f, 1.0f);
        float verticalInertiaDecayRate = std::clamp(inertiaDecayRate.Value().y, 0.0f, 1.0f);

        m_interactionTracker.PositionInertiaDecayRate(
            winrt::float3(horizontalInertiaDecayRate, verticalInertiaDecayRate, 0.0f));
    }

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_STR, METH_NAME, this,
        L"TryUpdatePositionWithAdditionalVelocity", TypeLogging::Float2ToString(winrt::float2(offsetsVelocity)).c_str());

    m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePositionWithAdditionalVelocity(
        winrt::float3(offsetsVelocity, 0.0f));
    m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity;
}

// Restores the default scroll inertia decay rate if no offset change with additional velocity operation is in progress.
void Scroller::PostProcessOffsetsChange(
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, interactionTrackerAsyncOperation);

    MUX_ASSERT(m_interactionTracker);

    if (interactionTrackerAsyncOperation->GetRequestId() != m_latestInteractionTrackerRequest)
    {
        std::shared_ptr<InteractionTrackerAsyncOperation> latestInteractionTrackerAsyncOperation = GetInteractionTrackerOperationFromRequestId(
            m_latestInteractionTrackerRequest);
        if (latestInteractionTrackerAsyncOperation &&
            latestInteractionTrackerAsyncOperation->GetOperationType() == InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity)
        {
            // Do not reset the scroll inertia decay rate when there is a new ongoing offset change with additional velocity
            return;
        }
    }

    m_interactionTracker.PositionInertiaDecayRate(nullptr);
}

// Launches an InteractionTracker request to change the zoomFactor.
void Scroller::ProcessZoomFactorChange(
    std::shared_ptr<ZoomFactorChange> zoomFactorChange,
    int32_t zoomFactorChangeId)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(zoomFactorChange);

    float zoomFactor = zoomFactorChange->ZoomFactor();
    winrt::IReference<winrt::float2> nullableCenterPoint = zoomFactorChange->CenterPoint();
    ScrollerViewKind viewKind = zoomFactorChange->ViewKind();
    winrt::ZoomOptions options = zoomFactorChange->Options().try_as<winrt::ZoomOptions>();

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        TypeLogging::ScrollerViewKindToString(viewKind).c_str(),
        zoomFactorChangeId);
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(nullableCenterPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactor);

    winrt::float2 centerPoint2D = nullableCenterPoint == nullptr ?
        winrt::float2(static_cast<float>(m_viewportWidth / 2.0), static_cast<float>(m_viewportHeight / 2.0)) : nullableCenterPoint.Value();
    winrt::float3 centerPoint(centerPoint2D.x - m_contentLayoutOffsetX, centerPoint2D.y - m_contentLayoutOffsetY, 0.0f);

    switch (viewKind)
    {
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
    case ScrollerViewKind::RelativeToEndOfInertiaView:
        {
            zoomFactor += ComputeEndOfInertiaZoomFactor();
            break;
        }
#endif
        case ScrollerViewKind::RelativeToCurrentView:
        {
            zoomFactor += m_zoomFactor;
            break;
        }
    }

    winrt::AnimationMode animationMode = options ? options.AnimationMode() : ScrollOptions::s_defaultAnimationMode;
    winrt::SnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollOptions::s_defaultSnapPointsMode;

    if (snapPointsMode == winrt::SnapPointsMode::Default)
    {
        zoomFactor = static_cast<float>(ComputeValueAfterSnapPoints<winrt::ZoomSnapPointBase>(zoomFactor, m_sortedConsolidatedZoomSnapPoints));
    }

    switch (animationMode)
    {
        case winrt::AnimationMode::Disabled:
        {
            SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_FLT_STR, METH_NAME, this, 
                L"TryUpdateScale", zoomFactor, TypeLogging::Float2ToString(winrt::float2(centerPoint.x, centerPoint.y)).c_str());

            m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdateScale(zoomFactor, centerPoint);
            m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdateScale;

            HookCompositionTargetRendering();
            break;
        }
        case winrt::AnimationMode::Enabled:
        {
            SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"TryUpdateScaleWithAnimation");

            m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdateScaleWithAnimation(
                GetZoomFactorAnimation(zoomFactor, centerPoint2D, zoomFactorChangeId),
                centerPoint);
            m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation;
            break;
        }
    }
}

// Launches an InteractionTracker request to change the zoomFactor with an additional velocity and an optional zoomFactor inertia decay rate.
void Scroller::ProcessZoomFactorChange(
    std::shared_ptr<ZoomFactorChangeWithAdditionalVelocity> zoomFactorChangeWithAdditionalVelocity)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(zoomFactorChangeWithAdditionalVelocity);

    float zoomFactorVelocity = zoomFactorChangeWithAdditionalVelocity->ZoomFactorVelocity();
    winrt::IReference<float> inertiaDecayRate = zoomFactorChangeWithAdditionalVelocity->InertiaDecayRate();
    winrt::IReference<winrt::float2> nullableCenterPoint = zoomFactorChangeWithAdditionalVelocity->CenterPoint();

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(nullableCenterPoint).c_str(),
        inertiaDecayRate,
        zoomFactorVelocity);

    if (inertiaDecayRate)
    {
        float scaleInertiaDecayRate = std::clamp(inertiaDecayRate.Value(), 0.0f, 1.0f);

        m_interactionTracker.ScaleInertiaDecayRate(scaleInertiaDecayRate);
    }

    winrt::float2 centerPoint2D = nullableCenterPoint == nullptr ?
        winrt::float2(static_cast<float>(m_viewportWidth / 2.0), static_cast<float>(m_viewportHeight / 2.0)) : nullableCenterPoint.Value();
    winrt::float3 centerPoint(centerPoint2D.x - m_contentLayoutOffsetX, centerPoint2D.y - m_contentLayoutOffsetY, 0.0f);

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_FLT_STR, METH_NAME, this,
        L"TryUpdateScaleWithAdditionalVelocity", zoomFactorVelocity, TypeLogging::Float2ToString(winrt::float2(centerPoint.x, centerPoint.y)).c_str());

    m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdateScaleWithAdditionalVelocity(
        zoomFactorVelocity,
        centerPoint);
    m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity;
}

// Restores the default zoomFactor inertia decay rate if no zoomFactor change with additional velocity operation is in progress.
void Scroller::PostProcessZoomFactorChange(
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation)
{
    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, interactionTrackerAsyncOperation);

    MUX_ASSERT(m_interactionTracker);

    if (interactionTrackerAsyncOperation->GetRequestId() != m_latestInteractionTrackerRequest)
    {
        std::shared_ptr<InteractionTrackerAsyncOperation> latestInteractionTrackerAsyncOperation = GetInteractionTrackerOperationFromRequestId(
            m_latestInteractionTrackerRequest);
        if (latestInteractionTrackerAsyncOperation &&
            latestInteractionTrackerAsyncOperation->GetOperationType() == InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity)
        {
            // Do not reset the zoomFactor inertia decay rate when there is a new ongoing zoomFactor change with additional velocity
            return;
        }
    }

    m_interactionTracker.ScaleInertiaDecayRate(nullptr);
}

// Workaround for RS5 InteractionTracker bug 18827625: Interrupt on-going TryUpdatePositionWithAnimation
// operation before launching new one. Interruption is done through a call to TryUpdatePositionBy(0, 0, 0).
// Similarly a call to TryUpdateScale is made to interrupt a previous call to TryUpdateScaleWithAnimation.
// Returns True when an interruption was performed.
bool Scroller::InterruptViewChangeWithAnimation(InteractionTrackerAsyncOperationType interactionTrackerAsyncOperationType)
{
    if (m_state == winrt::InteractionState::Animation &&
        interactionTrackerAsyncOperationType == m_lastInteractionTrackerAsyncOperationType &&
        SharedHelpers::IsRS5OrHigher() &&
        !SharedHelpers::Is19H1OrHigher())
    {
        MUX_ASSERT(m_interactionTracker);

        int interruptionId = 0;

        if (interactionTrackerAsyncOperationType == InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation)
        {
            interruptionId = m_interactionTracker.TryUpdatePositionBy(winrt::float3(0.0f));
        }
        else
        {
            MUX_ASSERT(interactionTrackerAsyncOperationType == InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation);
            interruptionId = m_interactionTracker.TryUpdateScale(m_zoomFactor, winrt::float3(0.0f));
        }

        SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, interruptionId);

        return true;
    }
    return false;
}

void Scroller::CompleteViewChange(
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation,
    ScrollerViewChangeResult result)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this,
        interactionTrackerAsyncOperation.get(), TypeLogging::ScrollerViewChangeResultToString(result).c_str());

    bool onHorizontalOffsetChangeCompleted = false;
    bool onVerticalOffsetChangeCompleted = false;

    switch (static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()))
    {
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::DirectViewChange):
            switch (interactionTrackerAsyncOperation->GetOperationType())
            {
            case InteractionTrackerAsyncOperationType::TryUpdatePosition:
            case InteractionTrackerAsyncOperationType::TryUpdatePositionBy:
            case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation:
            case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
                RaiseViewChangeCompleted(true /*isForScroll*/, result, interactionTrackerAsyncOperation->GetViewChangeId());
                break;
            default:
                RaiseViewChangeCompleted(false /*isForScroll*/, result, interactionTrackerAsyncOperation->GetViewChangeId());
                break;
            }
            break;
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest):
            onHorizontalOffsetChangeCompleted = true;
            break;
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest):
            onVerticalOffsetChangeCompleted = true;
            break;
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) |
            static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest):
            onHorizontalOffsetChangeCompleted = true;
            onVerticalOffsetChangeCompleted = true;
            break;
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::MouseWheel):
            break;
    }

    if (onHorizontalOffsetChangeCompleted && m_horizontalScrollController)
    {
        m_horizontalScrollController.get().OnScrollCompleted(
            winrt::ScrollInfo{ interactionTrackerAsyncOperation->GetViewChangeId() });
    }

    if (onVerticalOffsetChangeCompleted && m_verticalScrollController)
    {
        m_verticalScrollController.get().OnScrollCompleted(
            winrt::ScrollInfo{ interactionTrackerAsyncOperation->GetViewChangeId() });
    }
}

void Scroller::CompleteInteractionTrackerOperations(
    int requestId,
    ScrollerViewChangeResult operationResult,
    ScrollerViewChangeResult priorNonAnimatedOperationsResult,
    ScrollerViewChangeResult priorAnimatedOperationsResult,
    bool completeOperation,
    bool completePriorNonAnimatedOperations,
    bool completePriorAnimatedOperations)
{
    MUX_ASSERT(requestId != 0);
    MUX_ASSERT(completeOperation || completePriorNonAnimatedOperations || completePriorAnimatedOperations);

    if (m_interactionTrackerAsyncOperations.empty())
    {
        return;
    }

    for (auto operationsIter = m_interactionTrackerAsyncOperations.begin(); operationsIter != m_interactionTrackerAsyncOperations.end();)
    {
        auto& interactionTrackerAsyncOperation = *operationsIter;

        operationsIter++;

        bool isMatch = requestId == -1 || requestId == interactionTrackerAsyncOperation->GetRequestId();
        bool isPriorMatch = requestId > interactionTrackerAsyncOperation->GetRequestId() && -1 != interactionTrackerAsyncOperation->GetRequestId();

        if ((isPriorMatch && (completePriorNonAnimatedOperations || completePriorAnimatedOperations)) ||
            (isMatch && completeOperation))
        {
            bool isOperationAnimated = interactionTrackerAsyncOperation->IsAnimated();
            bool complete = (isMatch && completeOperation) ||
                (isPriorMatch && completePriorNonAnimatedOperations && !isOperationAnimated) ||
                (isPriorMatch && completePriorAnimatedOperations && isOperationAnimated);

            if (complete)
            {
                CompleteViewChange(
                    interactionTrackerAsyncOperation,
                    isMatch ? operationResult : (isOperationAnimated ? priorAnimatedOperationsResult : priorNonAnimatedOperationsResult));

                auto interactionTrackerAsyncOperationRemoved = interactionTrackerAsyncOperation;

                m_interactionTrackerAsyncOperations.remove(interactionTrackerAsyncOperationRemoved);

                switch (interactionTrackerAsyncOperationRemoved->GetOperationType())
                {
                    case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
                        PostProcessOffsetsChange(interactionTrackerAsyncOperationRemoved);
                        break;
                    case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity:
                        PostProcessZoomFactorChange(interactionTrackerAsyncOperationRemoved);
                        break;
                }
            }
        }
    }
}

void Scroller::CompleteDelayedOperations()
{
    if (m_interactionTrackerAsyncOperations.empty())
    {
        return;
    }

    SCROLLER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    for (auto operationsIter = m_interactionTrackerAsyncOperations.begin(); operationsIter != m_interactionTrackerAsyncOperations.end();)
    {
        auto& interactionTrackerAsyncOperation = *operationsIter;

        operationsIter++;

        if (interactionTrackerAsyncOperation->IsDelayed())
        {
            CompleteViewChange(interactionTrackerAsyncOperation, ScrollerViewChangeResult::Interrupted);
            m_interactionTrackerAsyncOperations.remove(interactionTrackerAsyncOperation);
        }
    }
}

int Scroller::GetInteractionTrackerOperationsTicksCountdownForTrigger(InteractionTrackerAsyncOperationTrigger operationTrigger) const
{
    int ticksCountdown = 0;

    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        if ((static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()) & static_cast<int>(operationTrigger)) != 0x00 &&
            !interactionTrackerAsyncOperation->IsCanceled())
        {
            ticksCountdown = std::max(ticksCountdown, interactionTrackerAsyncOperation->GetTicksCountdown());
        }
    }

    return ticksCountdown;
}

int Scroller::GetInteractionTrackerOperationsCount(bool includeAnimatedOperations, bool includeNonAnimatedOperations) const
{
    MUX_ASSERT(includeAnimatedOperations || includeNonAnimatedOperations);

    int operationsCount = 0;

    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        bool isOperationAnimated = interactionTrackerAsyncOperation->IsAnimated();

        if ((isOperationAnimated && includeAnimatedOperations) || (!isOperationAnimated && includeNonAnimatedOperations))
        {
            operationsCount++;
        }
    }

    return operationsCount;
}

std::shared_ptr<InteractionTrackerAsyncOperation> Scroller::GetInteractionTrackerOperationFromRequestId(int requestId) const
{
    MUX_ASSERT(requestId >= 0);

    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        if (interactionTrackerAsyncOperation->GetRequestId() == requestId)
        {
            return interactionTrackerAsyncOperation;
        }
    }

    return nullptr;
}

std::shared_ptr<InteractionTrackerAsyncOperation> Scroller::GetInteractionTrackerOperationFromKinds(
    bool isOperationTypeForOffsetsChange,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    ScrollerViewKind const& viewKind,
    winrt::ScrollOptions const& options) const
{
    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();

        if ((static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()) & static_cast<int>(operationTrigger)) == 0x00 ||
            !interactionTrackerAsyncOperation->IsQueued() ||
            interactionTrackerAsyncOperation->IsUnqueueing() ||
            interactionTrackerAsyncOperation->IsCanceled() ||
            !viewChangeBase)
        {
            continue;
        }

        switch (interactionTrackerAsyncOperation->GetOperationType())
        {
        case InteractionTrackerAsyncOperationType::TryUpdatePosition:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionBy:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation:
        {
            if (!isOperationTypeForOffsetsChange)
            {
                continue;
            }

            std::shared_ptr<ViewChange> viewChange = std::reinterpret_pointer_cast<ViewChange>(viewChangeBase);

            if (viewChange->ViewKind() != viewKind)
            {
                continue;
            }

            winrt::ScrollOptions optionsClone = viewChange->Options().try_as<winrt::ScrollOptions>();
            winrt::AnimationMode animationMode = options ? options.AnimationMode() : ScrollOptions::s_defaultAnimationMode;
            winrt::AnimationMode animationModeClone = optionsClone ? optionsClone.AnimationMode() : ScrollOptions::s_defaultAnimationMode;

            if (animationModeClone != animationMode)
            {
                continue;
            }

            winrt::SnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollOptions::s_defaultSnapPointsMode;
            winrt::SnapPointsMode snapPointsModeClone = optionsClone ? optionsClone.SnapPointsMode() : ScrollOptions::s_defaultSnapPointsMode;

            if (snapPointsModeClone != snapPointsMode)
            {
                continue;
            }
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdateScale:
        case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation:
        {
            if (isOperationTypeForOffsetsChange)
            {
                continue;
            }

            std::shared_ptr<ViewChange> viewChange = std::reinterpret_pointer_cast<ViewChange>(viewChangeBase);

            if (viewChange->ViewKind() != viewKind)
            {
                continue;
            }

            winrt::ZoomOptions optionsClone = viewChange->Options().try_as<winrt::ZoomOptions>();
            winrt::AnimationMode animationMode = options ? options.AnimationMode() : ScrollOptions::s_defaultAnimationMode;
            winrt::AnimationMode animationModeClone = optionsClone ? optionsClone.AnimationMode() : ScrollOptions::s_defaultAnimationMode;

            if (animationModeClone != animationMode)
            {
                continue;
            }

            winrt::SnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollOptions::s_defaultSnapPointsMode;
            winrt::SnapPointsMode snapPointsModeClone = optionsClone ? optionsClone.SnapPointsMode() : ScrollOptions::s_defaultSnapPointsMode;

            if (snapPointsModeClone != snapPointsMode)
            {
                continue;
            }
            break;
        }
        }

        return interactionTrackerAsyncOperation;
    }

    return nullptr;
}

std::shared_ptr<InteractionTrackerAsyncOperation> Scroller::GetInteractionTrackerOperationWithAdditionalVelocity(
    bool isOperationTypeForOffsetsChange,
    InteractionTrackerAsyncOperationTrigger operationTrigger) const
{
    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();

        if ((static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()) & static_cast<int>(operationTrigger)) == 0x00 ||
            !interactionTrackerAsyncOperation->IsQueued() ||
            interactionTrackerAsyncOperation->IsUnqueueing() ||
            interactionTrackerAsyncOperation->IsCanceled() ||
            !viewChangeBase)
        {
            continue;
        }

        switch (interactionTrackerAsyncOperation->GetOperationType())
        {
            case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
            {
                if (!isOperationTypeForOffsetsChange)
                {
                    continue;
                }
                return interactionTrackerAsyncOperation;
            }
            case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity:
            {
                if (isOperationTypeForOffsetsChange)
                {
                    continue;
                }
                return interactionTrackerAsyncOperation;
            }
        }
    }

    return nullptr;
}

winrt::InteractionTrackerInertiaRestingValue Scroller::GetInertiaRestingValue(
    winrt::SnapPointBase const& snapPoint,
    winrt::Compositor const& compositor,
    winrt::hstring const& target,
    winrt::hstring const& scale) const
{
    SnapPointBase* sp = winrt::get_self<SnapPointBase>(snapPoint);
    winrt::InteractionTrackerInertiaRestingValue modifier = winrt::InteractionTrackerInertiaRestingValue::Create(compositor);

    winrt::ExpressionAnimation conditionExpressionAnimation = sp->CreateConditionalExpression(compositor, target, scale);
    winrt::ExpressionAnimation restingPointExpressionAnimation = sp->CreateRestingPointExpression(compositor, target, scale);

    modifier.Condition(conditionExpressionAnimation);
    modifier.RestingValue(restingPointExpressionAnimation);

    return modifier;
}

bool Scroller::IsLoaded()
{
    return winrt::VisualTreeHelper::GetParent(*this) != nullptr;
}

bool Scroller::IsLoadedAndSetUp()
{
    return IsLoaded() && m_interactionTracker;
}

bool Scroller::IsInputKindIgnored(winrt::InputKind const& inputKind)
{
    return (IgnoredInputKind() & inputKind) == inputKind;
}

void Scroller::HookCompositionTargetRendering()
{
    if (!m_renderingToken)
    {
        winrt::Windows::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };
        m_renderingToken = compositionTarget.Rendering(winrt::auto_revoke, { this, &Scroller::OnCompositionTargetRendering });
    }
}

void Scroller::UnhookCompositionTargetRendering()
{
    m_renderingToken.revoke();    
}

void Scroller::HookScrollerEvents()
{
    if (!m_loadedToken)
    {
        m_loadedToken = Loaded(winrt::auto_revoke, { this, &Scroller::OnLoaded });
    }

    if (!m_unloadedToken)
    {
        m_unloadedToken = Unloaded(winrt::auto_revoke, { this, &Scroller::OnUnloaded });
    }

    if (SharedHelpers::IsRS4OrHigher() && !m_bringIntoViewRequested)
    {
        m_bringIntoViewRequested = BringIntoViewRequested(winrt::auto_revoke, { this, &Scroller::OnBringIntoViewRequestedHandler });
    }

    if (!Scroller::IsInteractionTrackerMouseWheelZoomingEnabled() && !m_pointerWheelChangedToken)
    {
        m_pointerWheelChangedToken = PointerWheelChanged(winrt::auto_revoke, { this, &Scroller::OnPointerWheelChangedHandler });
    }

    if (!m_pointerPressedEventHandler)
    {
        m_pointerPressedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &Scroller::OnPointerPressed });
        MUX_ASSERT(m_pointerPressedEventHandler);
        AddHandler(winrt::UIElement::PointerPressedEvent(), m_pointerPressedEventHandler, true /*handledEventsToo*/);
    }
}

void Scroller::UnhookScrollerEvents()
{
    m_loadedToken.revoke();
    m_unloadedToken.revoke();
    
    m_bringIntoViewRequested.revoke();
    
    if (m_pointerWheelChangedToken)
    {
        MUX_ASSERT(!Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());
        m_pointerWheelChangedToken.revoke();
    }

    if (m_pointerPressedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_pointerPressedEventHandler);
        m_pointerPressedEventHandler = nullptr;
    }
}

void Scroller::HookContentPropertyChanged(
    const winrt::UIElement& content)
{
    if (content)
    {
        if (!m_contentHorizontalAlignmentChangedToken)
        {
            m_contentHorizontalAlignmentChangedToken = RegisterPropertyChanged(content, winrt::FrameworkElement::HorizontalAlignmentProperty(), { this, &Scroller::OnContentPropertyChanged });
        }
        if (!m_contentVerticalAlignmentChangedToken)
        {
            m_contentVerticalAlignmentChangedToken = RegisterPropertyChanged(content, winrt::FrameworkElement::VerticalAlignmentProperty(), { this, &Scroller::OnContentPropertyChanged });
        }
    }
}

void Scroller::UnhookContentPropertyChanged(
    const winrt::UIElement& content)
{
    if (content)
    {
        const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

        if (contentAsFE)
        {
            m_contentHorizontalAlignmentChangedToken.revoke();
            m_contentVerticalAlignmentChangedToken.revoke();
        }
    }
}

void Scroller::HookHorizontalScrollControllerEvents(
    const winrt::IScrollController& horizontalScrollController,
    bool hasInteractionSource)
{
    MUX_ASSERT(horizontalScrollController);

    if (hasInteractionSource && !m_horizontalScrollControllerInteractionRequestedToken)
    {
        m_horizontalScrollControllerInteractionRequestedToken = horizontalScrollController.InteractionRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerInteractionRequested });
    }

    if (!m_horizontalScrollControllerInteractionInfoChangedToken)
    {
        m_horizontalScrollControllerInteractionInfoChangedToken = horizontalScrollController.InteractionInfoChanged(winrt::auto_revoke, { this, &Scroller::OnScrollControllerInteractionInfoChanged });
    }

    if (!m_horizontalScrollControllerScrollToRequestedToken)
    {
        m_horizontalScrollControllerScrollToRequestedToken = horizontalScrollController.ScrollToRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerScrollToRequested });
    }

    if (!m_horizontalScrollControllerScrollByRequestedToken)
    {
        m_horizontalScrollControllerScrollByRequestedToken = horizontalScrollController.ScrollByRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerScrollByRequested });
    }

    if (!m_horizontalScrollControllerScrollFromRequestedToken)
    {
        m_horizontalScrollControllerScrollFromRequestedToken = horizontalScrollController.ScrollFromRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerScrollFromRequested });
    }
}

void Scroller::HookVerticalScrollControllerEvents(
    const winrt::IScrollController& verticalScrollController,
    bool hasInteractionSource)
{
    MUX_ASSERT(verticalScrollController);

    if (hasInteractionSource && !m_verticalScrollControllerInteractionRequestedToken)
    {
        m_verticalScrollControllerInteractionRequestedToken = verticalScrollController.InteractionRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerInteractionRequested });
    }

    if (!m_verticalScrollControllerInteractionInfoChangedToken)
    {
        m_verticalScrollControllerInteractionInfoChangedToken = verticalScrollController.InteractionInfoChanged(winrt::auto_revoke, { this, &Scroller::OnScrollControllerInteractionInfoChanged });
    }

    if (!m_verticalScrollControllerScrollToRequestedToken)
    {
        m_verticalScrollControllerScrollToRequestedToken = verticalScrollController.ScrollToRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerScrollToRequested });
    }

    if (!m_verticalScrollControllerScrollByRequestedToken)
    {
        m_verticalScrollControllerScrollByRequestedToken = verticalScrollController.ScrollByRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerScrollByRequested });
    }

    if (!m_verticalScrollControllerScrollFromRequestedToken)
    {
        m_verticalScrollControllerScrollFromRequestedToken = verticalScrollController.ScrollFromRequested(winrt::auto_revoke, { this, &Scroller::OnScrollControllerScrollFromRequested });
    }
}

void Scroller::UnhookHorizontalScrollControllerEvents(
    const winrt::IScrollController& horizontalScrollController)
{
    MUX_ASSERT(horizontalScrollController);
    m_horizontalScrollControllerInteractionRequestedToken.revoke();
    m_horizontalScrollControllerInteractionInfoChangedToken.revoke();
    m_verticalScrollControllerScrollToRequestedToken.revoke();
    m_verticalScrollControllerScrollByRequestedToken.revoke();
    m_horizontalScrollControllerScrollFromRequestedToken.revoke();
}

void Scroller::UnhookVerticalScrollControllerEvents(
    const winrt::IScrollController& verticalScrollController)
{
    MUX_ASSERT(verticalScrollController);
    m_verticalScrollControllerInteractionRequestedToken.revoke();
    m_verticalScrollControllerInteractionInfoChangedToken.revoke();
    m_verticalScrollControllerScrollToRequestedToken.revoke();
    m_verticalScrollControllerScrollByRequestedToken.revoke();
    m_verticalScrollControllerScrollFromRequestedToken.revoke();
}

void Scroller::SetKeyEvents()
{
    MUX_ASSERT(SharedHelpers::IsRS4OrHigher());
    MUX_ASSERT(!Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());

    if (!m_isListeningToKeystrokes)
    {
#ifdef USE_INSIDER_SDK
        if (winrt::IUIElement10 uiElement10 = *this)
        {
            if (auto xamlRoot = uiElement10.XamlRoot())
            {
                auto xamlRootContent = xamlRoot.Content();

                m_onXamlRootKeyDownEventHandler.set(winrt::box_value<winrt::KeyEventHandler>({ this, &Scroller::OnXamlRootKeyDownOrUp }));
                xamlRootContent.AddHandler(winrt::UIElement::KeyDownEvent(), m_onXamlRootKeyDownEventHandler.get(), true);

                m_onXamlRootKeyUpEventHandler.set(winrt::box_value<winrt::KeyEventHandler>({ this, &Scroller::OnXamlRootKeyDownOrUp }));
                xamlRootContent.AddHandler(winrt::UIElement::KeyUpEvent(), m_onXamlRootKeyUpEventHandler.get(), true);
            }
        }
        else
#endif
        {
            auto coreWindow = winrt::Window::Current().CoreWindow();

            m_coreWindowKeyDownRevoker = coreWindow.KeyDown(
                winrt::auto_revoke,
                { this, &Scroller::OnCoreWindowKeyDownOrUp });

            m_coreWindowKeyUpRevoker = coreWindow.KeyUp(
                winrt::auto_revoke,
                { this, &Scroller::OnCoreWindowKeyDownOrUp });
        }

        m_isListeningToKeystrokes = true;
    }
}

void Scroller::ResetKeyEvents()
{
    MUX_ASSERT(SharedHelpers::IsRS4OrHigher());
    MUX_ASSERT(!Scroller::IsInteractionTrackerMouseWheelZoomingEnabled());

    if (m_isListeningToKeystrokes)
    {
#ifdef USE_INSIDER_SDK
        if (winrt::IUIElement10 uiElement10 = *this)
        {
            if (auto xamlRoot = uiElement10.XamlRoot())
            {
                auto xamlRootContent = xamlRoot.Content();

                if (m_onXamlRootKeyDownEventHandler)
                {
                    xamlRootContent.RemoveHandler(winrt::UIElement::KeyDownEvent(), m_onXamlRootKeyDownEventHandler.get());
                    m_onXamlRootKeyDownEventHandler.set(nullptr);
                }

                if (m_onXamlRootKeyUpEventHandler)
                {
                    xamlRootContent.RemoveHandler(winrt::UIElement::KeyUpEvent(), m_onXamlRootKeyUpEventHandler.get());
                    m_onXamlRootKeyUpEventHandler.set(nullptr);
                }
            }
        }
        else
#endif
        {
            m_coreWindowKeyDownRevoker = {};
            m_coreWindowKeyUpRevoker = {};
        }

        m_isListeningToKeystrokes = false;
    }
}

void Scroller::RaiseInteractionSourcesChanged()
{
    com_ptr<ScrollerTestHooks> globalTestHooks = ScrollerTestHooks::GetGlobalTestHooks();

    if (globalTestHooks && globalTestHooks->AreInteractionSourcesNotificationsRaised())
    {
        globalTestHooks->NotifyInteractionSourcesChanged(*this, m_interactionTracker.InteractionSources());
    }
}

void Scroller::RaiseExtentChanged()
{
    if (m_extentChangedEventSource)
    {
        SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        m_extentChangedEventSource(*this, nullptr);
    }
}

void Scroller::RaiseStateChanged()
{
    if (m_stateChangedEventSource)
    {
        SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        m_stateChangedEventSource(*this, nullptr);
    }
}

void Scroller::RaiseViewChanged()
{
    if (m_viewChangedEventSource)
    {
        SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangedEventSource(*this, nullptr);
    }

    if (!SharedHelpers::IsRS5OrHigher())
    {
        RaiseViewportChanged(false /* isFinal */);
    }

    if (SharedHelpers::IsFrameworkElementInvalidateViewportAvailable())
    {
        InvalidateViewport();
    }
}

winrt::CompositionAnimation Scroller::RaiseScrollAnimationStarting(
    const winrt::Vector3KeyFrameAnimation& positionAnimation,
    const winrt::float2& currentPosition,
    const winrt::float2& endPosition,
    int32_t offsetsChangeId)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, offsetsChangeId);
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_FLT_FLT_FLT_FLT, METH_NAME, this,
        currentPosition.x, currentPosition.y,
        endPosition.x, endPosition.y);

    if (m_scrollAnimationStartingEventSource)
    {
        auto scrollAnimationStartingEventArgs = winrt::make_self<ScrollAnimationStartingEventArgs>();

        if (offsetsChangeId != -1)
        {
            scrollAnimationStartingEventArgs->SetOffsetsChangeId(offsetsChangeId);
        }

        scrollAnimationStartingEventArgs->SetAnimation(positionAnimation);
        scrollAnimationStartingEventArgs->SetStartPosition(currentPosition);
        scrollAnimationStartingEventArgs->SetEndPosition(endPosition);
        m_scrollAnimationStartingEventSource(*this, *scrollAnimationStartingEventArgs);
        return scrollAnimationStartingEventArgs->GetAnimation();
    }
    else
    {
        return positionAnimation;
    }
}

winrt::CompositionAnimation Scroller::RaiseZoomAnimationStarting(
    const winrt::ScalarKeyFrameAnimation& zoomFactorAnimation,
    const float endZoomFactor,
    const winrt::float2& centerPoint,
    int32_t zoomFactorChangeId)
{
    SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_FLT_FLT_STR_INT, METH_NAME, this,
        m_zoomFactor,
        endZoomFactor,
        TypeLogging::Float2ToString(centerPoint).c_str(),
        zoomFactorChangeId);

    if (m_zoomAnimationStartingEventSource)
    {
        auto zoomAnimationStartingEventArgs = winrt::make_self<ZoomAnimationStartingEventArgs>();

        if (zoomFactorChangeId != -1)
        {
            zoomAnimationStartingEventArgs->SetZoomFactorChangeId(zoomFactorChangeId);
        }

        zoomAnimationStartingEventArgs->SetAnimation(zoomFactorAnimation);
        zoomAnimationStartingEventArgs->SetCenterPoint(centerPoint);
        zoomAnimationStartingEventArgs->SetStartZoomFactor(m_zoomFactor);
        zoomAnimationStartingEventArgs->SetEndZoomFactor(endZoomFactor);
        m_zoomAnimationStartingEventSource(*this, *zoomAnimationStartingEventArgs);
        return zoomAnimationStartingEventArgs->GetAnimation();
    }
    else
    {
        return zoomFactorAnimation;
    }
}

void Scroller::RaiseViewChangeCompleted(
    bool isForScroll,
    ScrollerViewChangeResult result,
    int32_t viewChangeId)
{
    if (viewChangeId)
    {
        if (isForScroll && m_scrollCompletedEventSource)
        {
            SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
                TypeLogging::ScrollerViewChangeResultToString(result).c_str(),
                viewChangeId);

            auto scrollCompletedEventArgs = winrt::make_self<ScrollCompletedEventArgs>();

            scrollCompletedEventArgs->Result(result);
            scrollCompletedEventArgs->OffsetsChangeId(viewChangeId);
            m_scrollCompletedEventSource(*this, *scrollCompletedEventArgs);
        }
        else if (!isForScroll && m_zoomCompletedEventSource)
        {
            SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
                TypeLogging::ScrollerViewChangeResultToString(result).c_str(),
                viewChangeId);

            auto zoomCompletedEventArgs = winrt::make_self<ZoomCompletedEventArgs>();

            zoomCompletedEventArgs->Result(result);
            zoomCompletedEventArgs->ZoomFactorChangeId(viewChangeId);
            m_zoomCompletedEventSource(*this, *zoomCompletedEventArgs);
        }
    }

    // Raise viewport changed only when effective viewport
    // support is not available.
    if (SharedHelpers::IsRS5OrHigher())
    {
        RaiseViewportChanged(true /* isFinal */);
    }

    if (SharedHelpers::IsFrameworkElementInvalidateViewportAvailable())
    {
        InvalidateViewport();
    }
}

// Returns False when ScrollerBringingIntoViewEventArgs.Cancel is set to True to skip the operation.
bool Scroller::RaiseBringingIntoView(
    double targetZoomedHorizontalOffset,
    double targetZoomedVerticalOffset,
    const winrt::BringIntoViewRequestedEventArgs& requestEventArgs,
    int32_t offsetsChangeId,
    _Inout_ winrt::SnapPointsMode* snapPointsMode)
{
    if (m_bringingIntoViewEventSource)
    {
        SCROLLER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        auto bringingIntoViewEventArgs = winrt::make_self<ScrollerBringingIntoViewEventArgs>();

        bringingIntoViewEventArgs->SnapPointsMode(*snapPointsMode);
        bringingIntoViewEventArgs->OffsetsChangeId(offsetsChangeId);
        bringingIntoViewEventArgs->RequestEventArgs(requestEventArgs);
        bringingIntoViewEventArgs->TargetOffsets(targetZoomedHorizontalOffset, targetZoomedVerticalOffset);

        m_bringingIntoViewEventSource(*this, *bringingIntoViewEventArgs);
        *snapPointsMode = bringingIntoViewEventArgs->SnapPointsMode();
        return !bringingIntoViewEventArgs->Cancel();
    }
    return true;
}

#ifdef _DEBUG
void Scroller::DumpMinMaxPositions()
{
    MUX_ASSERT(m_interactionTracker);

    const winrt::UIElement content = Content();

    if (!content)
    {
        // Min/MaxPosition == (0, 0)
        return;
    }

    const winrt::Visual scrollerVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();
    float minPosX = 0.0f;
    float minPosY = 0.0f;
    float extentWidth = static_cast<float>(m_unzoomedExtentWidth);
    float extentHeight = static_cast<float>(m_unzoomedExtentHeight);

    if (contentAsFE)
    {
        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            minPosX = std::min(0.0f, (extentWidth * m_interactionTracker.Scale() - scrollerVisual.Size().x) / 2.0f);
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            minPosX = std::min(0.0f, extentWidth * m_interactionTracker.Scale() - scrollerVisual.Size().x);
        }

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            minPosY = std::min(0.0f, (extentHeight * m_interactionTracker.Scale() - scrollerVisual.Size().y) / 2.0f);
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            minPosY = std::min(0.0f, extentHeight * m_interactionTracker.Scale() - scrollerVisual.Size().y);
        }
    }

    float maxPosX = std::max(0.0f, extentWidth * m_interactionTracker.Scale() - scrollerVisual.Size().x);
    float maxPosY = std::max(0.0f, extentHeight * m_interactionTracker.Scale() - scrollerVisual.Size().y);

    if (contentAsFE)
    {
        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            maxPosX = (extentWidth * m_interactionTracker.Scale() - scrollerVisual.Size().x) >= 0 ?
                (extentWidth * m_interactionTracker.Scale() - scrollerVisual.Size().x) : (extentWidth * m_interactionTracker.Scale() - scrollerVisual.Size().x) / 2.0f;
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            maxPosX = extentWidth * m_interactionTracker.Scale() - scrollerVisual.Size().x;
        }

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            maxPosY = (extentHeight * m_interactionTracker.Scale() - scrollerVisual.Size().y) >= 0 ?
                (extentHeight * m_interactionTracker.Scale() - scrollerVisual.Size().y) : (extentHeight * m_interactionTracker.Scale() - scrollerVisual.Size().y) / 2.0f;
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            maxPosY = extentHeight * m_interactionTracker.Scale() - scrollerVisual.Size().y;
        }
    }
}
#endif // _DEBUG

#ifdef _DEBUG

winrt::hstring Scroller::DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_ContentProperty)
    {
        return L"Content";
    }
    else if (dependencyProperty == s_BackgroundProperty)
    {
        return L"Background";
    }
    else if (dependencyProperty == s_ContentOrientationProperty)
    {
        return L"ContentOrientation";
    }
    else if (dependencyProperty == s_VerticalScrollChainingModeProperty)
    {
        return L"VerticalScrollChainingMode";
    }
    else if (dependencyProperty == s_ZoomChainingModeProperty)
    {
        return L"ZoomChainingMode";
    }
    else if (dependencyProperty == s_HorizontalScrollRailingModeProperty)
    {
        return L"HorizontalScrollRailingMode";
    }
    else if (dependencyProperty == s_VerticalScrollRailingModeProperty)
    {
        return L"VerticalScrollRailingMode";
    }
    else if (dependencyProperty == s_HorizontalScrollModeProperty)
    {
        return L"HorizontalScrollMode";
    }
    else if (dependencyProperty == s_VerticalScrollModeProperty)
    {
        return L"VerticalScrollMode";
    }
#ifdef USE_SCROLLMODE_AUTO
    else if (dependencyProperty == s_ComputedHorizontalScrollModeProperty)
    {
        return L"ComputedHorizontalScrollMode";
    }
    else if (dependencyProperty == s_ComputedVerticalScrollModeProperty)
    {
        return L"ComputedVerticalScrollMode";
    }
#endif
    else if (dependencyProperty == s_ZoomModeProperty)
    {
        return L"ZoomMode";
    }
    else if (dependencyProperty == s_IgnoredInputKindProperty)
    {
        return L"IgnoredInputKind";
    }
    else if (dependencyProperty == s_MinZoomFactorProperty)
    {
        return L"MinZoomFactor";
    }
    else if (dependencyProperty == s_MaxZoomFactorProperty)
    {
        return L"MaxZoomFactor";
    }
    else if (dependencyProperty == s_HorizontalAnchorRatioProperty)
    {
        return L"HorizontalAnchorRatio";
    }
    else if (dependencyProperty == s_VerticalAnchorRatioProperty)
    {
        return L"VerticalAnchorRatio";
    }
    else
    {
        return L"UNKNOWN";
    }
}

#endif
