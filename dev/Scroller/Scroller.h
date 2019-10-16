// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FloatUtil.h"
#include "InteractionTrackerAsyncOperation.h"
#include "ScrollAnimationStartingEventArgs.h"
#include "ZoomAnimationStartingEventArgs.h"
#include "ScrollCompletedEventArgs.h"
#include "ZoomCompletedEventArgs.h"
#include "ScrollerBringingIntoViewEventArgs.h"
#include "ScrollerAnchorRequestedEventArgs.h"
#include "SnapPointWrapper.h"
#include "ScrollerTrace.h"
#include "ViewChange.h"
#include "OffsetsChange.h"
#include "OffsetsChangeWithAdditionalVelocity.h"
#include "ZoomFactorChange.h"
#include "ZoomFactorChangeWithAdditionalVelocity.h"

#include "Scroller.g.h"
#include "Scroller.properties.h"

class Scroller :
    public ReferenceTracker<Scroller, DeriveFromPanelHelper_base, winrt::Scroller, winrt::Controls::IScrollAnchorProvider, winrt::IRepeaterScrollingSurface>,
    public ScrollerProperties
{
public:
    Scroller();
    ~Scroller();

    // Background property is ambiguous with Panel, lift up ScrollerProperties::Background to disambiguate.
    using ScrollerProperties::Background;

    // Properties of the ExpressionAnimationSources CompositionPropertySet
    static constexpr std::wstring_view s_extentSourcePropertyName{ L"Extent"sv };
    static constexpr std::wstring_view s_viewportSourcePropertyName{ L"Viewport"sv };
    static constexpr std::wstring_view s_offsetSourcePropertyName{ L"Offset"sv };
    static constexpr std::wstring_view s_positionSourcePropertyName{ L"Position"sv };
    static constexpr std::wstring_view s_minPositionSourcePropertyName{ L"MinPosition"sv };
    static constexpr std::wstring_view s_maxPositionSourcePropertyName{ L"MaxPosition"sv };
    static constexpr std::wstring_view s_zoomFactorSourcePropertyName{ L"ZoomFactor"sv };

    // Properties' default values.
    static constexpr winrt::ChainingMode s_defaultHorizontalScrollChainingMode{ winrt::ChainingMode::Auto };
    static constexpr winrt::ChainingMode s_defaultVerticalScrollChainingMode{ winrt::ChainingMode::Auto };
    static constexpr winrt::RailingMode s_defaultHorizontalScrollRailingMode{ winrt::RailingMode::Enabled };
    static constexpr winrt::RailingMode s_defaultVerticalScrollRailingMode{ winrt::RailingMode::Enabled };
#ifdef USE_SCROLLMODE_AUTO
    static constexpr winrt::ScrollMode s_defaultHorizontalScrollMode{ winrt::ScrollMode::Auto };
    static constexpr winrt::ScrollMode s_defaultVerticalScrollMode{ winrt::ScrollMode::Auto };
    static constexpr winrt::ScrollMode s_defaultComputedHorizontalScrollMode{ winrt::ScrollMode::Disabled };
    static constexpr winrt::ScrollMode s_defaultComputedVerticalScrollMode{ winrt::ScrollMode::Disabled };
#else
    static constexpr winrt::ScrollMode s_defaultHorizontalScrollMode{ winrt::ScrollMode::Enabled };
    static constexpr winrt::ScrollMode s_defaultVerticalScrollMode{ winrt::ScrollMode::Enabled };
#endif
    static constexpr winrt::ChainingMode s_defaultZoomChainingMode{ winrt::ChainingMode::Auto };
    static constexpr winrt::ZoomMode s_defaultZoomMode{ winrt::ZoomMode::Disabled };
    static constexpr winrt::InputKind s_defaultIgnoredInputKind{ winrt::InputKind::None };
    static constexpr winrt::ContentOrientation s_defaultContentOrientation{ winrt::ContentOrientation::None };
    static constexpr bool s_defaultAnchorAtExtent{ true };
    static constexpr double s_defaultMinZoomFactor{ 0.1 };
    static constexpr double s_defaultMaxZoomFactor{ 10.0 };
    static constexpr double s_defaultAnchorRatio{ 0.0 };

    // ChangeOffsets scrolling constants
    static constexpr int s_offsetsChangeMsPerUnit{ 5 };
    static constexpr int s_offsetsChangeMinMs{ 50 };
    static constexpr int s_offsetsChangeMaxMs{ 1000 };

    // ChangeZoomFactor zooming constants
    static constexpr int s_zoomFactorChangeMsPerUnit{ 250 };
    static constexpr int s_zoomFactorChangeMinMs{ 50 };
    static constexpr int s_zoomFactorChangeMaxMs{ 1000 };

    // Number of ticks ellapsed before restarting the Translation and Scale animations to allow the Content
    // rasterization to be triggered after the Idle State is reached or a zoom factor change operation completed.
    static constexpr int s_translationAndZoomFactorAnimationsRestartTicks = 4;

    // Mouse-wheel-triggered scrolling/zooming constants
    // Mouse wheel delta amount required per initial velocity unit
    // 120 matches the built-in InteractionTracker scrolling/zooming behavior introduced in RS5.
    static constexpr int32_t s_mouseWheelDeltaForVelocityUnit = 120;
    // Inertia decay rate to achieve the c_zoomFactorChangePerVelocityUnit=0.1f zoom factor change per velocity unit
    static constexpr float s_mouseWheelInertiaDecayRateRS1 = 0.997361f;
    // 0.999972 closely matches the built-in InteractionTracker scrolling/zooming behavior introduced in RS5.
    static constexpr float s_mouseWheelInertiaDecayRate = 0.999972f;

    static const winrt::ScrollInfo s_noOpScrollInfo;
    static const winrt::ZoomInfo s_noOpZoomInfo;

#pragma region IScrollAnchorProvider
    void RegisterAnchorCandidate(winrt::UIElement const& element);
    void UnregisterAnchorCandidate(winrt::UIElement const& element);
    winrt::UIElement CurrentAnchor();

    // To be removed
    winrt::Controls::IScrollAnchorProvider Parent() { return nullptr; }
    void Parent(winrt::Controls::IScrollAnchorProvider const& value) {}
#pragma endregion

#pragma region IRepeaterScrollingSurface
    bool IsHorizontallyScrollable();

    bool IsVerticallyScrollable();

    winrt::UIElement AnchorElement();

    winrt::event_token ViewportChanged(winrt::ViewportChangedEventHandler const& value);

    void ViewportChanged(winrt::event_token const& token);

    winrt::event_token PostArrange(winrt::PostArrangeEventHandler const& value);

    void PostArrange(winrt::event_token const& token);

    winrt::event_token ConfigurationChanged(winrt::ConfigurationChangedEventHandler const& value);

    void ConfigurationChanged(winrt::event_token const& token);

    winrt::Rect GetRelativeViewport(
        winrt::UIElement const& content);
#pragma endregion

#pragma region IFrameworkElementOverridesHelper
    // IFrameworkElementOverrides (unoverridden methods provided by FrameworkElementOverridesHelper)
    winrt::Size MeasureOverride(winrt::Size const& availableSize); // not actually final for 'derived' classes
    winrt::Size ArrangeOverride(winrt::Size const& finalSize); // not actually final for 'derived' classes
#pragma endregion

#pragma region InteractionTrackerOwner callbacks
    void ValuesChanged(const winrt::InteractionTrackerValuesChangedArgs& args);
    void RequestIgnored(const winrt::InteractionTrackerRequestIgnoredArgs& args);
    void InteractingStateEntered(const winrt::InteractionTrackerInteractingStateEnteredArgs& args);
    void InertiaStateEntered(const winrt::InteractionTrackerInertiaStateEnteredArgs& args);
    void IdleStateEntered(const winrt::InteractionTrackerIdleStateEnteredArgs& args);
    void CustomAnimationStateEntered(const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args);
#pragma endregion

#pragma region IScroller
    winrt::CompositionPropertySet ExpressionAnimationSources();

    double HorizontalOffset();
    double VerticalOffset();
    float ZoomFactor();
    double ExtentWidth();
    double ExtentHeight();
    double ViewportWidth();
    double ViewportHeight();
    double ScrollableWidth();
    double ScrollableHeight();

    winrt::IScrollController HorizontalScrollController();
    void HorizontalScrollController(winrt::IScrollController const& value);

    winrt::IScrollController VerticalScrollController();
    void VerticalScrollController(winrt::IScrollController const& value);

    winrt::InputKind IgnoredInputKind();
    void IgnoredInputKind(winrt::InputKind const& value);

    winrt::InteractionState State();

    winrt::IVector<winrt::ScrollSnapPointBase> HorizontalSnapPoints();

    winrt::IVector<winrt::ScrollSnapPointBase> VerticalSnapPoints();

    winrt::IVector<winrt::ZoomSnapPointBase> ZoomSnapPoints();

    winrt::ScrollInfo ScrollTo(double horizontalOffset, double verticalOffset);
    winrt::ScrollInfo ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollOptions const& options);
    winrt::ScrollInfo ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta);
    winrt::ScrollInfo ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollOptions const& options);
    winrt::ScrollInfo ScrollFrom(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate);
    winrt::ZoomInfo ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint);
    winrt::ZoomInfo ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ZoomOptions const& options);
    winrt::ZoomInfo ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint);
    winrt::ZoomInfo ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ZoomOptions const& options);
    winrt::ZoomInfo ZoomFrom(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate);

#pragma endregion

    enum class ScrollerDimension
    {
        HorizontalScroll,
        VerticalScroll,
        HorizontalZoomFactor,
        VerticalZoomFactor,
        Scroll,
        ZoomFactor
    };

    // Invoked by both Scroller and ScrollViewer controls
    static bool IsZoomFactorBoundaryValid(double value);
    static void ValidateZoomFactoryBoundary(double value);

    // Invoked by both Scroller and ScrollViewer controls
    static bool IsAnchorRatioValid(double value);
    static void ValidateAnchorRatio(double value);

    bool IsElementValidAnchor(
        const winrt::UIElement& element);

    // Invoked by ScrollerTestHooks
    float GetContentLayoutOffsetX() const
    {
        return m_contentLayoutOffsetX;
    }

    float GetContentLayoutOffsetY() const
    {
        return m_contentLayoutOffsetY;
    }

    void SetContentLayoutOffsetX(float contentLayoutOffsetX);
    void SetContentLayoutOffsetY(float contentLayoutOffsetY);

    winrt::float2 GetArrangeRenderSizesDelta();
    winrt::float2 GetMinPosition();
    winrt::float2 GetMaxPosition();

    winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedHorizontalScrollSnapPoints()
    {
        return GetConsolidatedScrollSnapPoints(ScrollerDimension::HorizontalScroll);
    }

    winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedVerticalScrollSnapPoints()
    {
        return GetConsolidatedScrollSnapPoints(ScrollerDimension::VerticalScroll);
    }

    winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedScrollSnapPoints(ScrollerDimension dimension);
    winrt::IVector<winrt::ZoomSnapPointBase> GetConsolidatedZoomSnapPoints();

    SnapPointWrapper<winrt::ScrollSnapPointBase>* GetHorizontalSnapPointWrapper(winrt::ScrollSnapPointBase const& scrollSnapPoint)
    {
        return GetScrollSnapPointWrapper(ScrollerDimension::HorizontalScroll, scrollSnapPoint);
    }

    SnapPointWrapper<winrt::ScrollSnapPointBase>* GetVerticalSnapPointWrapper(winrt::ScrollSnapPointBase const& scrollSnapPoint)
    {
        return GetScrollSnapPointWrapper(ScrollerDimension::VerticalScroll, scrollSnapPoint);
    }

    SnapPointWrapper<winrt::ScrollSnapPointBase>* GetScrollSnapPointWrapper(ScrollerDimension dimension, winrt::ScrollSnapPointBase const& scrollSnapPoint);
    SnapPointWrapper<winrt::ZoomSnapPointBase>* GetZoomSnapPointWrapper(winrt::ZoomSnapPointBase const& zoomSnapPoint);

    // Invoked when a dependency property of this Scroller has changed.
    void OnPropertyChanged(
        const winrt::DependencyPropertyChangedEventArgs& args);

    void OnContentSizeChanged(
        const winrt::IInspectable& sender,
        const winrt::SizeChangedEventArgs& args);
    void OnContentPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);

#pragma region Automation Peer Helpers
    // Public methods accessed by the CScrollerAutomationPeer class
    double GetZoomedExtentWidth() const;
    double GetZoomedExtentHeight() const;

    void PageLeft();
    void PageRight();
    void PageUp();
    void PageDown();
    void LineLeft();
    void LineRight();
    void LineUp();
    void LineDown();
    void ScrollToHorizontalOffset(double offset);
    void ScrollToVerticalOffset(double offset);
    void ScrollToOffsets(double horizontalOffset, double verticalOffset);
#pragma endregion

    // IUIElementOverridesHelper
    winrt::AutomationPeer OnCreateAutomationPeer();

private:
#ifdef _DEBUG
    static winrt::hstring DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty);
#endif

    winrt::Size ArrangeContent(
        const winrt::UIElement& content,
        const winrt::Thickness& contentMargin,
        winrt::Rect& finalContentRect,
        bool wasContentArrangeWidthStretched,
        bool wasContentArrangeHeightStretched);
    float ComputeContentLayoutOffsetDelta(ScrollerDimension dimension, float unzoomedDelta) const;
    float ComputeEndOfInertiaZoomFactor() const;
    winrt::float2 ComputeEndOfInertiaPosition();
    void ComputeMinMaxPositions(float zoomFactor, _Out_opt_ winrt::float2* minPosition, _Out_opt_ winrt::float2* maxPosition);
    winrt::float2 ComputePositionFromOffsets(double zoomedHorizontalOffset, double zoomedVerticalOffset);
    template <typename T> double ComputeValueAfterSnapPoints(double value, std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>> const& snapPointsSet);
    winrt::float2 ComputeCenterPointerForMouseWheelZooming(const winrt::UIElement& content, const winrt::Point& pointerPosition) const;
    void ComputeBringIntoViewTargetOffsets(
        const winrt::UIElement& content,
        const winrt::SnapPointsMode& snapPointsMode,
        const winrt::BringIntoViewRequestedEventArgs& requestEventArgs,
        _Out_ double* targetZoomedHorizontalOffset,
        _Out_ double* targetZoomedVerticalOffset,
        _Out_ double* appliedOffsetX,
        _Out_ double* appliedOffsetY,
        _Out_ winrt::Rect* targetRect);

    void EnsureExpressionAnimationSources();
    void EnsureInteractionTracker();
    void EnsureScrollerVisualInteractionSource();
    void EnsureScrollControllerVisualInteractionSource(
        const winrt::Visual& interactionVisual,
        ScrollerDimension dimension);
    void EnsureScrollControllerExpressionAnimationSources(
        ScrollerDimension dimension);
    void EnsurePositionBoundariesExpressionAnimations();
    void EnsureTransformExpressionAnimations();
    template <typename T> void SetupSnapPoints(
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
        ScrollerDimension dimension);
    template <typename T> void UpdateSnapPointsRanges(
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
        bool forImpulseOnly);
    template <typename T> void UpdateSnapPointsIgnoredValue(
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
        ScrollerDimension dimension);
    template <typename T> bool UpdateSnapPointsIgnoredValue(
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
        double newIgnoredValue);
    template <typename T> void UpdateSnapPointsInertiaFromImpulse(
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
        ScrollerDimension dimension,
        bool isInertiaFromImpulse);
    void SetupInteractionTrackerBoundaries();
    void SetupInteractionTrackerZoomFactorBoundaries(
        double minZoomFactor, double maxZoomFactor);
    void SetupScrollerVisualInteractionSource();
    void SetupScrollControllerVisualInterationSource(
        ScrollerDimension dimension);
    void SetupScrollControllerVisualInterationSourcePositionModifiers(
        ScrollerDimension dimension,
        const winrt::Orientation& orientation);
    void SetupVisualInteractionSourceRailingMode(
        const winrt::VisualInteractionSource& visualInteractionSource,
        ScrollerDimension dimension,
        const winrt::RailingMode& railingMode);
    void SetupVisualInteractionSourceChainingMode(
        const winrt::VisualInteractionSource& visualInteractionSource,
        ScrollerDimension dimension,
        const winrt::ChainingMode& chainingMode);
    void SetupVisualInteractionSourceMode(
        const winrt::VisualInteractionSource& visualInteractionSource,
        ScrollerDimension dimension,
        const winrt::ScrollMode& scrollMode);
    void SetupVisualInteractionSourceMode(
        const winrt::VisualInteractionSource& visualInteractionSource,
        const winrt::ZoomMode& zoomMode);
#ifdef IsMouseWheelScrollDisabled
    void SetupVisualInteractionSourcePointerWheelConfig(
        const winrt::VisualInteractionSource& visualInteractionSource,
        ScrollerDimension dimension,
        const winrt::ScrollMode& scrollMode);
#endif
#ifdef IsMouseWheelZoomDisabled
    void SetupVisualInteractionSourcePointerWheelConfig(
        const winrt::VisualInteractionSource& visualInteractionSource,
        const winrt::ZoomMode& zoomMode);
#endif
    void SetupVisualInteractionSourceRedirectionMode(
        const winrt::VisualInteractionSource& visualInteractionSource);
    void SetupVisualInteractionSourceCenterPointModifier(
        const winrt::VisualInteractionSource& visualInteractionSource,
        ScrollerDimension dimension);
    void SetupPositionBoundariesExpressionAnimations(
        const winrt::UIElement& content);
    void SetupTransformExpressionAnimations(
        const winrt::UIElement& content);
    void StartTransformExpressionAnimations(
        const winrt::UIElement& content,
        bool forAnimationsInterruption);
    void StopTransformExpressionAnimations(
        const winrt::UIElement& content,
        bool forAnimationsInterruption);
    bool StartTranslationAndZoomFactorExpressionAnimations(bool interruptCountdown = false);
    void StopTranslationAndZoomFactorExpressionAnimations();
    void StartExpressionAnimationSourcesAnimations();
    void StartScrollControllerExpressionAnimationSourcesAnimations(
        ScrollerDimension dimension);
    void StopScrollControllerExpressionAnimationSourcesAnimations(
        ScrollerDimension dimension);
    void UpdateContent(
        const winrt::UIElement& oldContent,
        const winrt::UIElement& newContent);
    void UpdatePositionBoundaries(
        const winrt::UIElement& content);
    void UpdateTransformSource(
        const winrt::UIElement& oldContent,
        const winrt::UIElement& newContent);
    void UpdateState(
        const winrt::InteractionState& state);
    void UpdateExpressionAnimationSources();
    void UpdateUnzoomedExtentAndViewport(
        bool renderSizeChanged,
        double unzoomedExtentWidth,
        double unzoomedExtentHeight,
        double viewportWidth,
        double viewportHeight);
    void UpdateScrollAutomationPatternProperties();
    void UpdateIsInertiaFromImpulse(bool isInertiaFromImpulse);
    void UpdateOffset(ScrollerDimension dimension, double zoomedOffset);
    void UpdateScrollControllerInteractionsAllowed(ScrollerDimension dimension);
    void UpdateScrollControllerValues(ScrollerDimension dimension);
    void UpdateVisualInteractionSourceMode(ScrollerDimension dimension);
    void UpdateManipulationRedirectionMode();
    void UpdateDisplayInformation(winrt::DisplayInformation const& displayInformation);
    void OnContentSizeChanged(
        const winrt::UIElement& content);
    void OnViewChanged(bool horizontalOffsetChanged, bool verticalOffsetChanged);
    void OnContentLayoutOffsetChanged(ScrollerDimension dimension);

    void ChangeOffsetsPrivate(
        double zoomedHorizontalOffset,
        double zoomedVerticalOffset,
        ScrollerViewKind offsetsKind,
        winrt::ScrollOptions const& options,
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        int32_t existingViewChangeId,
        _Out_opt_ int32_t* viewChangeId);
    void ChangeOffsetsWithAdditionalVelocityPrivate(
        winrt::float2 offsetsVelocity,
        winrt::float2 anticipatedOffsetsChange,
        winrt::IReference<winrt::float2> inertiaDecayRate,
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        _Out_opt_ int32_t* viewChangeId);

    void ChangeZoomFactorPrivate(
        float zoomFactor,
        winrt::IReference<winrt::float2> centerPoint,
        ScrollerViewKind zoomFactorKind,
        winrt::ZoomOptions const& options,
        _Out_opt_ int32_t* viewChangeId);
    void ChangeZoomFactorWithAdditionalVelocityPrivate(
        float zoomFactorVelocity,
        float anticipatedZoomFactorChange,
        winrt::IReference<winrt::float2> centerPoint,
        winrt::IReference<float> inertiaDecayRate,
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        _Out_opt_ int32_t* viewChangeId);

    void ProcessPointerWheelScroll(
        bool isHorizontalMouseWheel,
        int32_t mouseWheelDelta,
        float anticipatedEndOfInertiaPosition,
        float minPosition,
        float maxPosition);
    void ProcessPointerWheelZoom(
        winrt::PointerPoint const& pointerPoint,
        int32_t mouseWheelDelta,
        float anticipatedEndOfInertiaZoomFactor,
        float minZoomFactor,
        float maxZoomFactor);
    void ProcessDequeuedViewChange(
        std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation);
    void ProcessOffsetsChange(
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        std::shared_ptr<OffsetsChange> offsetsChange,
        int32_t offsetsChangeId,
        bool isForAsyncOperation);
    void ProcessOffsetsChange(
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity);
    void PostProcessOffsetsChange(
        std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation);
    void ProcessZoomFactorChange(
        std::shared_ptr<ZoomFactorChange> zoomFactorChange,
        int32_t zoomFactorChangeId);
    void ProcessZoomFactorChange(
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        std::shared_ptr<ZoomFactorChangeWithAdditionalVelocity> zoomFactorChangeWithAdditionalVelocity);
    void PostProcessZoomFactorChange(
        std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation);
    bool InterruptViewChangeWithAnimation(InteractionTrackerAsyncOperationType interactionTrackerAsyncOperationType);
    void CompleteViewChange(
        std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation,
        ScrollerViewChangeResult result);
    void CompleteInteractionTrackerOperations(
        int requestId,
        ScrollerViewChangeResult operationResult,
        ScrollerViewChangeResult priorNonAnimatedOperationsResult,
        ScrollerViewChangeResult priorAnimatedOperationsResult,
        bool completeNonAnimatedOperation,
        bool completeAnimatedOperation,
        bool completePriorNonAnimatedOperations,
        bool completePriorAnimatedOperations);
    void CompleteDelayedOperations();
    winrt::float2 GetMouseWheelAnticipatedOffsetsChange() const;
    float GetMouseWheelAnticipatedZoomFactorChange() const;
    int GetInteractionTrackerOperationsTicksCountdownForTrigger(
        InteractionTrackerAsyncOperationTrigger operationTrigger) const;
    int GetInteractionTrackerOperationsCount(
        bool includeAnimatedOperations,
        bool includeNonAnimatedOperations) const;
    std::shared_ptr<InteractionTrackerAsyncOperation> GetLastNonAnimatedInteractionTrackerOperation(
        std::shared_ptr<InteractionTrackerAsyncOperation> priorToInteractionTrackerOperation) const;
    std::shared_ptr<InteractionTrackerAsyncOperation> GetInteractionTrackerOperationFromRequestId(
        int requestId) const;
    std::shared_ptr<InteractionTrackerAsyncOperation> GetInteractionTrackerOperationFromKinds(
        bool isOperationTypeForOffsetsChange,
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        ScrollerViewKind const& viewKind,
        winrt::ScrollOptions const& options) const;
    std::shared_ptr<InteractionTrackerAsyncOperation> GetInteractionTrackerOperationWithAdditionalVelocity(
        bool isOperationTypeForOffsetsChange,
        InteractionTrackerAsyncOperationTrigger operationTrigger) const;
    template <typename T> winrt::InteractionTrackerInertiaRestingValue GetInertiaRestingValue(
        std::shared_ptr<SnapPointWrapper<T>> snapPointWrapper,
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale) const;

#ifdef USE_SCROLLMODE_AUTO
    winrt::ScrollMode GetComputedScrollMode(ScrollerDimension dimension, bool ignoreZoomMode = false);
#endif
#ifdef IsMouseWheelScrollDisabled
    winrt::ScrollMode GetComputedMouseWheelScrollMode(ScrollerDimension dimension);
#endif
#ifdef IsMouseWheelZoomDisabled
    winrt::ZoomMode GetMouseWheelZoomMode();
#endif

    double GetComputedMaxWidth(
        double defaultMaxWidth,
        const winrt::FrameworkElement& content) const;
    double GetComputedMaxHeight(
        double defaultMaxHeight,
        const winrt::FrameworkElement& content) const;
    winrt::float2 GetArrangeRenderSizesDelta(
        const winrt::UIElement& content) const;
    winrt::hstring GetMinPositionExpression(
        const winrt::UIElement& content) const;
    winrt::hstring GetMinPositionXExpression(
        const winrt::UIElement& content) const;
    winrt::hstring GetMinPositionYExpression(
        const winrt::UIElement& content) const;
    winrt::hstring GetMaxPositionExpression(
        const winrt::UIElement& content) const;
    winrt::hstring GetMaxPositionXExpression(
        const winrt::UIElement& content) const;
    winrt::hstring GetMaxPositionYExpression(
        const winrt::UIElement& content) const;

    winrt::CompositionAnimation GetPositionAnimation(
        double zoomedHorizontalOffset,
        double zoomedVerticalOffset,
        InteractionTrackerAsyncOperationTrigger operationTrigger,
        int32_t offsetsChangeId);
    winrt::CompositionAnimation GetZoomFactorAnimation(
        float zoomFactor,
        const winrt::float2& centerPoint,
        int32_t zoomFactorChangeId);
    int GetNextViewChangeId();

    bool IsInertiaFromImpulse() const;
    bool IsLoadedAndSetUp() const;
    bool IsInputKindIgnored(winrt::InputKind const& inputKind);
    bool HasBringingIntoViewListener() const
    {
        return !!m_bringingIntoViewEventSource;
    }

    void HookCompositionTargetRendering();
    void HookDpiChangedEvent();
    void HookScrollerEvents();
    void HookContentPropertyChanged(
        const winrt::UIElement& content);
    void HookHorizontalScrollControllerEvents(
        const winrt::IScrollController& horizontalScrollController,
        bool hasInteractionVisual);
    void HookVerticalScrollControllerEvents(
        const winrt::IScrollController& verticalScrollController,
        bool hasInteractionVisual);
    void UnhookCompositionTargetRendering();
    void UnhookContentPropertyChanged(
        const winrt::UIElement& content);
    void UnhookScrollerEvents();
    void UnhookHorizontalScrollControllerEvents(
        const winrt::IScrollController& horizontalScrollController);
    void UnhookVerticalScrollControllerEvents(
        const winrt::IScrollController& verticalScrollController);

    void RaiseInteractionSourcesChanged();
    void RaiseExpressionAnimationStatusChanged(
        bool isExpressionAnimationStarted,
        wstring_view const& propertyName);
    void RaiseExtentChanged();
    void RaiseStateChanged();
    void RaiseViewChanged();
    winrt::CompositionAnimation RaiseScrollAnimationStarting(
        const winrt::Vector3KeyFrameAnimation& positionAnimation,
        const winrt::float2& currentPosition,
        const winrt::float2& endPosition,
        int32_t offsetsChangeId);
    winrt::CompositionAnimation RaiseZoomAnimationStarting(
        const winrt::ScalarKeyFrameAnimation& zoomFactorAnimation,
        const float endZoomFactor,
        const winrt::float2& centerPoint,
        int32_t zoomFactorChangeId);
    void RaiseViewChangeCompleted(
        bool isForScroll,
        ScrollerViewChangeResult result,
        int32_t viewChangeId);
    bool RaiseBringingIntoView(
        double targetZoomedHorizontalOffset,
        double targetZoomedVerticalOffset,
        const winrt::BringIntoViewRequestedEventArgs& requestEventArgs,
        int32_t offsetsChangeId,
        _Inout_ winrt::SnapPointsMode* snapPointsMode);

    // Event handlers
    void OnDpiChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnCompositionTargetRendering(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnLoaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    void OnUnloaded(
        const winrt::IInspectable &sender,
        const winrt::RoutedEventArgs &args);
    void OnBringIntoViewRequestedHandler(
        const winrt::IInspectable& sender,
        const winrt::BringIntoViewRequestedEventArgs& args);
    void OnPointerWheelChangedHandler(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnPointerPressed(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);

    void OnScrollControllerInteractionRequested(
        const winrt::IScrollController& sender,
        const winrt::ScrollControllerInteractionRequestedEventArgs& args);
    void OnScrollControllerInteractionInfoChanged(
        const winrt::IScrollController& sender,
        const winrt::IInspectable& args);
    void OnScrollControllerScrollToRequested(
        const winrt::IScrollController& sender,
        const winrt::ScrollControllerScrollToRequestedEventArgs& args);
    void OnScrollControllerScrollByRequested(
        const winrt::IScrollController& sender,
        const winrt::ScrollControllerScrollByRequestedEventArgs& args);
    void OnScrollControllerScrollFromRequested(
        const winrt::IScrollController& sender,
        const winrt::ScrollControllerScrollFromRequestedEventArgs& args);

    void OnHorizontalSnapPointsVectorChanged(
        const winrt::IObservableVector<winrt::ScrollSnapPointBase>& sender,
        const winrt::IVectorChangedEventArgs event);
    void OnVerticalSnapPointsVectorChanged(
        const winrt::IObservableVector<winrt::ScrollSnapPointBase>& sender,
        const winrt::IVectorChangedEventArgs event);
    void OnZoomSnapPointsVectorChanged(
        const winrt::IObservableVector<winrt::ZoomSnapPointBase>& sender,
        const winrt::IVectorChangedEventArgs event);

    template <typename T> bool SnapPointsViewportChangedHelper(
        winrt::IObservableVector<T> const& snapPoints,
        double viewport);
    template <typename T> void SnapPointsVectorChangedHelper(
        winrt::IObservableVector<T> const& scrollSnapPoints,
        winrt::IVectorChangedEventArgs const& args,
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
        ScrollerDimension dimension);
    template <typename T> void SnapPointsVectorItemInsertedHelper(
        std::shared_ptr<SnapPointWrapper<T>> insertedItem,
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet);
    template <typename T> void RegenerateSnapPointsSet(
        winrt::IObservableVector<T> const& userVector,
        std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* internalSet);

#pragma region IRepeaterScrollingSurface Helpers
    void RaiseConfigurationChanged();
    void RaisePostArrange();
    void RaiseViewportChanged(const bool isFinal);
    void RaiseAnchorRequested();

    void IsAnchoring(
        _Out_ bool* isAnchoringElementHorizontally,
        _Out_ bool* isAnchoringElementVertically,
        _Out_opt_ bool* isAnchoringFarEdgeHorizontally = nullptr,
        _Out_opt_ bool* isAnchoringFarEdgeVertically = nullptr);
    void ComputeViewportAnchorPoint(
        double viewportWidth,
        double viewportHeight,
        _Out_ double* viewportAnchorPointHorizontalOffset,
        _Out_ double* viewportAnchorPointVerticalOffset);
    void ComputeElementAnchorPoint(
        bool isForPreArrange,
        _Out_ double* elementAnchorPointHorizontalOffset,
        _Out_ double* elementAnchorPointVerticalOffset);
    void ComputeAnchorPoint(
        const winrt::Rect& anchorBounds,
        _Out_ double* anchorPointX,
        _Out_ double* anchorPointY);
    winrt::Size ComputeViewportToElementAnchorPointsDistance(
        double viewportWidth,
        double viewportHeight,
        bool isForPreArrange);
    void ClearAnchorCandidates();
    void ResetAnchorElement();
    void EnsureAnchorElementSelection();

    void ProcessAnchorCandidate(
        const winrt::UIElement& anchorCandidate,
        const winrt::UIElement& content,
        const winrt::Rect& viewportAnchorBounds,
        double viewportAnchorPointHorizontalOffset,
        double viewportAnchorPointVerticalOffset,
        _Inout_ double* bestAnchorCandidateDistance,
        _Inout_ winrt::UIElement* bestAnchorCandidate,
        _Inout_ winrt::Rect* bestAnchorCandidateBounds) const;

    static winrt::Rect GetDescendantBounds(
        const winrt::UIElement& content,
        const winrt::UIElement& descendant);

    static bool IsElementValidAnchor(
        const winrt::UIElement& element,
        const winrt::UIElement& content);
#pragma endregion

    static winrt::InteractionChainingMode InteractionChainingModeFromChainingMode(
        const winrt::ChainingMode& chainingMode);
#ifdef IsMouseWheelScrollDisabled
    static winrt::InteractionSourceRedirectionMode InteractionSourceRedirectionModeFromScrollMode(
        const winrt::ScrollMode& scrollMode);
#endif
#ifdef IsMouseWheelZoomDisabled
    static winrt::InteractionSourceRedirectionMode InteractionSourceRedirectionModeFromZoomMode(
        const winrt::ZoomMode& zoomMode);
#endif
    static winrt::InteractionSourceMode InteractionSourceModeFromScrollMode(
        const winrt::ScrollMode& scrollMode);
    static winrt::InteractionSourceMode InteractionSourceModeFromZoomMode(
        const winrt::ZoomMode& zoomMode);

    static double ComputeZoomedOffsetWithMinimalChange(
        double viewportStart,
        double viewportEnd,
        double childStart,
        double childEnd);

    static winrt::Rect GetDescendantBounds(
        const winrt::UIElement& content,
        const winrt::UIElement& descendant,
        const winrt::Rect& descendantRect);

    static winrt::AnimationMode GetComputedAnimationMode(
        winrt::AnimationMode const& animationMode);

    static bool IsInteractionTrackerPointerWheelRedirectionEnabled();
    static bool IsVisualTranslationPropertyAvailable();
    static wstring_view GetVisualTargetedPropertyName(ScrollerDimension dimension);

#ifdef _DEBUG
    void DumpMinMaxPositions();
#endif // _DEBUG

private:
    int m_latestViewChangeId{ 0 };
    int m_latestInteractionTrackerRequest{ 0 };
    InteractionTrackerAsyncOperationType m_lastInteractionTrackerAsyncOperationType{ InteractionTrackerAsyncOperationType::None };
    winrt::float2 m_endOfInertiaPosition{ 0.0f, 0.0f };
    float m_animationRestartZoomFactor{ 1.0f };
    float m_endOfInertiaZoomFactor{ 1.0f };
    float m_zoomFactor{ 1.0f };
    float m_contentLayoutOffsetX{ 0.0f };
    float m_contentLayoutOffsetY{ 0.0f };
    double m_zoomedHorizontalOffset{ 0.0 };
    double m_zoomedVerticalOffset{ 0.0 };
    double m_unzoomedExtentWidth{ 0.0 };
    double m_unzoomedExtentHeight{ 0.0 };
    double m_viewportWidth{ 0.0 };
    double m_viewportHeight{ 0.0 };
    bool m_horizontalSnapPointsNeedViewportUpdates{ false }; // True when at least one horizontal snap point is not near aligned.
    bool m_verticalSnapPointsNeedViewportUpdates{ false }; // True when at least one vertical snap point is not near aligned.
    bool m_isAnchorElementDirty{ true }; // False when m_anchorElement is up-to-date, True otherwise.
    bool m_isInertiaFromImpulse{ false }; // Only used on pre-RS5 versions, as a replacement for the InteractionTracker.IsInertiaFromImpulse property.

    // Display information used for mouse-wheel scrolling on pre-RS5 Windows versions.
    double m_rawPixelsPerViewPixel{};
    uint32_t m_screenWidthInRawPixels{};
    uint32_t m_screenHeightInRawPixels{};

    // Number of ticks remaining before restarting the Translation and Scale animations to allow the Content
    // rasterization to be triggered after the Idle State is reached or a zoom factor change operation completed.
    uint8_t m_translationAndZoomFactorAnimationsRestartTicksCountdown{};

    // For perf reasons, the value of ContentOrientation is cached.
    winrt::ContentOrientation m_contentOrientation{ s_defaultContentOrientation };
    winrt::Size m_availableSize{};

    tracker_ref<winrt::IScrollController> m_horizontalScrollController{ this };
    tracker_ref<winrt::IScrollController> m_verticalScrollController{ this };
    tracker_ref<winrt::UIElement> m_anchorElement{ this };
    tracker_ref<winrt::ScrollerAnchorRequestedEventArgs> m_anchorRequestedEventArgs{ this };
    std::vector<tracker_ref<winrt::UIElement>> m_anchorCandidates;
    std::list<std::shared_ptr<InteractionTrackerAsyncOperation>> m_interactionTrackerAsyncOperations;
    winrt::Rect m_anchorElementBounds{};
    winrt::InteractionState m_state{ winrt::InteractionState::Idle };
    winrt::IInspectable m_pointerPressedEventHandler{ nullptr };
    winrt::CompositionPropertySet m_expressionAnimationSources{ nullptr };
    winrt::CompositionPropertySet m_horizontalScrollControllerExpressionAnimationSources{ nullptr };
    winrt::CompositionPropertySet m_verticalScrollControllerExpressionAnimationSources{ nullptr };
    winrt::VisualInteractionSource m_scrollerVisualInteractionSource{ nullptr };
    winrt::VisualInteractionSource m_horizontalScrollControllerVisualInteractionSource{ nullptr };
    winrt::VisualInteractionSource m_verticalScrollControllerVisualInteractionSource{ nullptr };
    winrt::InteractionTracker m_interactionTracker{ nullptr };
    winrt::IInteractionTrackerOwner m_interactionTrackerOwner{ nullptr };
    winrt::ExpressionAnimation m_minPositionExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_maxPositionExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_translationExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_transformMatrixTranslateXExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_transformMatrixTranslateYExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_zoomFactorExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_transformMatrixZoomFactorExpressionAnimation{ nullptr };

    winrt::ExpressionAnimation m_positionSourceExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_minPositionSourceExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_maxPositionSourceExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_zoomFactorSourceExpressionAnimation{ nullptr };

    winrt::ExpressionAnimation m_horizontalScrollControllerOffsetExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_horizontalScrollControllerMaxOffsetExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_verticalScrollControllerOffsetExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_verticalScrollControllerMaxOffsetExpressionAnimation{ nullptr };

    // Event Sources
    event_source<winrt::ViewportChangedEventHandler> m_viewportChanged{ this };
    event_source<winrt::PostArrangeEventHandler> m_postArrange{ this };
    event_source<winrt::ConfigurationChangedEventHandler> m_configurationChanged{ this };

    // Event Revokers
    winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering_revoker m_renderingRevoker{};
    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    winrt::UIElement::BringIntoViewRequested_revoker m_bringIntoViewRequestedRevoker{};
    winrt::UIElement::PointerWheelChanged_revoker m_pointerWheelChangedRevoker{};
    PropertyChanged_revoker m_contentMinWidthChangedRevoker{};
    PropertyChanged_revoker m_contentWidthChangedRevoker{};
    PropertyChanged_revoker m_contentMaxWidthChangedRevoker{};
    PropertyChanged_revoker m_contentMinHeightChangedRevoker{};
    PropertyChanged_revoker m_contentHeightChangedRevoker{};
    PropertyChanged_revoker m_contentMaxHeightChangedRevoker{};
    PropertyChanged_revoker m_contentHorizontalAlignmentChangedRevoker{};
    PropertyChanged_revoker m_contentVerticalAlignmentChangedRevoker{};

    winrt::IScrollController::ScrollToRequested_revoker m_horizontalScrollControllerScrollToRequestedRevoker{};
    winrt::IScrollController::ScrollByRequested_revoker m_horizontalScrollControllerScrollByRequestedRevoker{};
    winrt::IScrollController::ScrollFromRequested_revoker m_horizontalScrollControllerScrollFromRequestedRevoker{};
    winrt::IScrollController::InteractionRequested_revoker m_horizontalScrollControllerInteractionRequestedRevoker{};
    winrt::IScrollController::InteractionInfoChanged_revoker m_horizontalScrollControllerInteractionInfoChangedRevoker{};

    winrt::IScrollController::ScrollToRequested_revoker m_verticalScrollControllerScrollToRequestedRevoker{};
    winrt::IScrollController::ScrollByRequested_revoker m_verticalScrollControllerScrollByRequestedRevoker{};
    winrt::IScrollController::ScrollFromRequested_revoker m_verticalScrollControllerScrollFromRequestedRevoker{};
    winrt::IScrollController::InteractionRequested_revoker m_verticalScrollControllerInteractionRequestedRevoker{};
    winrt::IScrollController::InteractionInfoChanged_revoker m_verticalScrollControllerInteractionInfoChangedRevoker{};

    // Used for mouse-wheel scrolling on pre-RS5 Windows versions.
    winrt::DisplayInformation::DpiChanged_revoker m_dpiChangedRevoker{};

    // Used on platforms where we don't have XamlRoot.
    winrt::ICoreWindow::KeyDown_revoker m_coreWindowKeyDownRevoker{};
    winrt::ICoreWindow::KeyUp_revoker m_coreWindowKeyUpRevoker{};

    winrt::IObservableVector<winrt::ScrollSnapPointBase>::VectorChanged_revoker m_horizontalSnapPointsVectorChangedRevoker{};
    winrt::IObservableVector<winrt::ScrollSnapPointBase>::VectorChanged_revoker m_verticalSnapPointsVectorChangedRevoker{};
    winrt::IObservableVector<winrt::ZoomSnapPointBase>::VectorChanged_revoker m_zoomSnapPointsVectorChangedRevoker{};

    winrt::IVector<winrt::ScrollSnapPointBase> m_horizontalSnapPoints{};
    winrt::IVector<winrt::ScrollSnapPointBase> m_verticalSnapPoints{};
    winrt::IVector<winrt::ZoomSnapPointBase> m_zoomSnapPoints{};
    std::set<std::shared_ptr<SnapPointWrapper<winrt::ScrollSnapPointBase>>, SnapPointWrapperComparator<winrt::ScrollSnapPointBase>> m_sortedConsolidatedHorizontalSnapPoints{};
    std::set<std::shared_ptr<SnapPointWrapper<winrt::ScrollSnapPointBase>>, SnapPointWrapperComparator<winrt::ScrollSnapPointBase>> m_sortedConsolidatedVerticalSnapPoints{};
    std::set<std::shared_ptr<SnapPointWrapper<winrt::ZoomSnapPointBase>>, SnapPointWrapperComparator<winrt::ZoomSnapPointBase>> m_sortedConsolidatedZoomSnapPoints{};

    // Maximum difference for offsets to be considered equal. Used for pointer wheel scrolling.
    static constexpr float s_offsetEqualityEpsilon{ 0.00001f };
    // Maximum difference for zoom factors to be considered equal. Used for pointer wheel zooming.
    static constexpr float s_zoomFactorEqualityEpsilon{ 0.00001f };

    // Property names being targeted for the Scroller.Content's Visual.
    // RedStone v1 case:
    static constexpr std::wstring_view s_transformMatrixTranslateXPropertyName{ L"TransformMatrix._41"sv };
    static constexpr std::wstring_view s_transformMatrixTranslateYPropertyName{ L"TransformMatrix._42"sv };
    static constexpr std::wstring_view s_transformMatrixScaleXPropertyName{ L"TransformMatrix._11"sv };
    static constexpr std::wstring_view s_transformMatrixScaleYPropertyName{ L"TransformMatrix._22"sv };
    // RedStone v2 and higher case:
    static constexpr std::wstring_view s_translationPropertyName{ L"Translation"sv };
    static constexpr std::wstring_view s_scalePropertyName{ L"Scale"sv };

    // Properties of the IScrollController's ExpressionAnimationSources CompositionPropertySet
    static constexpr wstring_view s_minOffsetPropertyName{ L"MinOffset"sv };
    static constexpr wstring_view s_maxOffsetPropertyName{ L"MaxOffset"sv };
    static constexpr wstring_view s_offsetPropertyName{ L"Offset"sv };
    static constexpr wstring_view s_multiplierPropertyName{ L"Multiplier"sv };

    // Properties used in snap points composition expressions
    static constexpr wstring_view s_naturalRestingPositionXPropertyName{ L"NaturalRestingPosition.x"sv };
    static constexpr wstring_view s_naturalRestingPositionYPropertyName{ L"NaturalRestingPosition.y"sv };
    static constexpr wstring_view s_naturalRestingScalePropertyName{ L"NaturalRestingScale"sv };
    static constexpr wstring_view s_targetScalePropertyName{ L"this.Target.Scale"sv };
};
