#pragma once

#include "pch.h"
#include "common.h"

#include "TeachingTipTemplateSettings.h"

#include "TeachingTip.g.h"
#include "TeachingTip.properties.h"

class TeachingTip :
    public ReferenceTracker<TeachingTip, winrt::implementation::TeachingTipT>,
    public TeachingTipProperties
{

public:
    TeachingTip();
    ~TeachingTip();

    // IFrameworkElement
    void OnApplyTemplate();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // ContentControl
    void OnContentChanged(const winrt::IInspectable& oldContent, const winrt::IInspectable& newContent);

    // UIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    tracker_ref<winrt::FrameworkElement> m_target{ this };

    // TestHooks
    void SetExpandEasingFunction(const winrt::CompositionEasingFunction& easingFunction);
    void SetContractEasingFunction(const winrt::CompositionEasingFunction& easingFunction);
    void SetTipShouldHaveShadow(bool tipShadow);
    void SetContentElevation(float elevation);
    void SetTailElevation(float elevation);
    bool GetIsIdle();
    winrt::TeachingTipPlacementMode GetEffectivePlacement();
    winrt::TeachingTipHeroContentPlacementMode GetEffectiveHeroContentPlacement();
    double GetHorizontalOffset();
    double GetVerticalOffset();
    void SetUseTestWindowBounds(bool useTestWindowBounds);
    void SetTestWindowBounds(const winrt::Rect& testWindowBounds);
    void SetUseTestScreenBounds(bool useTestScreenBounds);
    void SetTestScreenBounds(const winrt::Rect& testScreenBounds);
    void SetTipFollowsTarget(bool tipFollowsTarget);
    void SetReturnTopForOutOfWindowPlacement(bool useScreenBoundsForAutoPlacement);
    void SetExpandAnimationDuration(const winrt::TimeSpan& expandAnimationDuration);
    void SetContractAnimationDuration(const winrt::TimeSpan& contractAnimationDuration);

    bool m_isIdle{ true };

private:
    PropertyChanged_revoker m_automationNameChangedRevoker{};
    PropertyChanged_revoker m_automationIdChangedRevoker{};
    winrt::CoreDispatcher::AcceleratorKeyActivated_revoker m_acceleratorKeyActivatedRevoker{};
    winrt::UIElement::GettingFocus_revoker m_closeButtonGettingFocusFromF6Revoker{};
    winrt::Button::Click_revoker m_closeButtonClickedRevoker{};
    winrt::Button::Click_revoker m_alternateCloseButtonClickedRevoker{};
    winrt::Button::Click_revoker m_actionButtonClickedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_contentSizeChangedRevoker{};
    winrt::FrameworkElement::EffectiveViewportChanged_revoker m_effectiveViewportChangedRevoker{};
    winrt::FrameworkElement::EffectiveViewportChanged_revoker m_targetEffectiveViewportChangedRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_targetLayoutUpdatedRevoker{};
    winrt::Popup::Opened_revoker m_popupOpenedRevoker{};
    winrt::Popup::Closed_revoker m_popupClosedRevoker{};
    winrt::Popup::Closed_revoker m_lightDismissIndicatorPopupClosedRevoker{};
    winrt::Window::SizeChanged_revoker m_windowSizeChangedRevoker{};
    winrt::Grid::Loaded_revoker m_tailOcclusionGridLoadedRevoker{};
	void SetPopupAutomationProperties();
    void CreateLightDismissIndicatorPopup();
    bool UpdateTail();
    void PositionPopup();
    bool PositionTargetedPopup();
    bool PositionUntargetedPopup();
    void UpdateSizeBasedTemplateSettings();
    void UpdateButtonsState();
    void UpdateDynamicHeroContentPlacementToTop();
    void UpdateDynamicHeroContentPlacementToBottom();

    static void OnPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    void OnIsOpenChanged();
    void OnTargetChanged();
    void OnTailVisibilityChanged();
    void OnIconSourceChanged();
    void OnPlacementMarginChanged();
    void OnIsLightDismissEnabledChanged();
    void OnShouldConstrainToRootBoundsChanged();
    void OnHeroContentPlacementChanged();

    void OnAutomationNameChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnAutomationIdChanged(const winrt::IInspectable&, const winrt::IInspectable&);

    void OnF6AcceleratorKeyClicked(const winrt::CoreDispatcher&, const winrt::AcceleratorKeyEventArgs& args);
    void OnCloseButtonGettingFocusFromF6(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args);
    void OnCloseButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void OnActionButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void OnPopupOpened(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnLightDismissIndicatorPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnTailOcclusionGridLoaded(const winrt::IInspectable&, const winrt::IInspectable&);

    void RaiseClosingEvent(bool attachDeferralCompletedHandler);
    void ClosePopupWithAnimationIfAvailable();
    void ClosePopup();

    void SetViewportChangedEvent();
    void RevokeViewportChangedEvent();
    void TargetLayoutUpdated(const winrt::IInspectable&, const winrt::IInspectable&);

    void CreateExpandAnimation();
    void CreateContractAnimation();

    void StartExpandToOpen();
    void StartContractToClose();

    std::tuple<winrt::TeachingTipPlacementMode, bool> DetermineEffectivePlacement();
    std::tuple<winrt::Rect, winrt::Thickness, winrt::Thickness> DetermineSpaceAroundTarget();
    winrt::Rect GetEffectiveWindowBounds();
    winrt::Rect GetEffectiveScreenBounds();
    static std::array<winrt::TeachingTipPlacementMode, 13> GetPlacementFallbackOrder(winrt::TeachingTipPlacementMode preferredPalcement);
    void EstablishShadows();

    tracker_ref<winrt::Border> m_container{ this };

    tracker_ref<winrt::Popup> m_popup{ this };
    tracker_ref<winrt::Popup> m_lightDismissIndicatorPopup{ this };
    tracker_ref<winrt::ContentControl> m_popupContentControl{ this };

    tracker_ref<winrt::UIElement> m_rootElement{ this };
    tracker_ref<winrt::Grid> m_tailOcclusionGrid{ this };
    tracker_ref<winrt::Grid> m_contentRootGrid{ this };
    tracker_ref<winrt::Grid> m_nonHeroContentRootGrid{ this };
    tracker_ref<winrt::Border> m_heroContentBorder{ this };
    tracker_ref<winrt::Border> m_iconBorder{ this };
    tracker_ref<winrt::Button> m_actionButton{ this };
    tracker_ref<winrt::Button> m_alternateCloseButton{ this };
    tracker_ref<winrt::Button> m_closeButton{ this };
    tracker_ref<winrt::Polygon> m_tailPolygon{ this };
    tracker_ref<winrt::Grid> m_tailEdgeBorder{ this };

    tracker_ref<winrt::Control> m_previouslyFocusedElement{ this };

    tracker_ref<winrt::KeyFrameAnimation> m_expandAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_contractAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_expandElevationAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_contractElevationAnimation{ this };
    tracker_ref<winrt::CompositionEasingFunction> m_expandEasingFunction{ this };
    tracker_ref<winrt::CompositionEasingFunction> m_contractEasingFunction{ this };

    winrt::TeachingTipPlacementMode m_currentEffectiveTipPlacementMode{ winrt::TeachingTipPlacementMode::Auto };
    winrt::TeachingTipPlacementMode m_currentEffectiveTailPlacementMode{ winrt::TeachingTipPlacementMode::Auto };
    winrt::TeachingTipHeroContentPlacementMode m_currentHeroContentEffectivePlacementMode{ winrt::TeachingTipHeroContentPlacementMode::Auto };

    winrt::Rect m_currentBounds{ 0,0,0,0 };
    winrt::Rect m_currentTargetBounds{ 0,0,0,0 };

    bool m_isTemplateApplied{ false };
    bool m_createNewPopupOnOpen{ false };

    bool m_isExpandAnimationPlaying{ false };
    bool m_isContractAnimationPlaying{ false };

    bool m_useTestWindowBounds{ false };
    winrt::Rect m_testWindowBounds{ 0,0,0,0 };
    bool m_useTestScreenBounds{ false };
    winrt::Rect m_testScreenBounds{ 0,0,0,0 };

    bool m_tipShouldHaveShadow{ true };

    bool m_tipFollowsTarget{ false };
    bool m_returnTopForOutOfWindowPlacement{ true };

    float m_contentElevation{ 32.0f };
    float m_tailElevation{ 0.0f };
    bool m_tailShadowTargetsShadowTarget{ false };

    winrt::TimeSpan m_expandAnimationDuration{ 300ms };
    winrt::TimeSpan m_contractAnimationDuration{ 200ms };

    winrt::TeachingTipCloseReason m_lastCloseReason{ winrt::TeachingTipCloseReason::Programmatic };

    static bool IsPlacementTop(winrt::TeachingTipPlacementMode placement) {
        return placement == winrt::TeachingTipPlacementMode::Top ||
            placement == winrt::TeachingTipPlacementMode::TopLeft ||
            placement == winrt::TeachingTipPlacementMode::TopRight;
    }
    static bool IsPlacementBottom(winrt::TeachingTipPlacementMode placement) {
        return placement == winrt::TeachingTipPlacementMode::Bottom ||
            placement == winrt::TeachingTipPlacementMode::BottomLeft ||
            placement == winrt::TeachingTipPlacementMode::BottomRight;
    }
    static bool IsPlacementLeft(winrt::TeachingTipPlacementMode placement) {
        return placement == winrt::TeachingTipPlacementMode::Left ||
            placement == winrt::TeachingTipPlacementMode::LeftTop ||
            placement == winrt::TeachingTipPlacementMode::LeftBottom;
    }
    static bool IsPlacementRight(winrt::TeachingTipPlacementMode placement) {
        return placement == winrt::TeachingTipPlacementMode::Right ||
            placement == winrt::TeachingTipPlacementMode::RightTop ||
            placement == winrt::TeachingTipPlacementMode::RightBottom;
    }

    // These values are shifted by one because this is the 1px highlight that sits adjacent to the tip border.
    inline winrt::Thickness BottomPlacementTopRightHighlightMargin(double width, double height) { return { (width / 2) + (TailShortSideLength() - 1.0f), 0, 1, 0 }; }
    inline winrt::Thickness BottomRightPlacementTopRightHighlightMargin(double width, double height) { return { MinimumTipEdgeToTailEdgeMargin() + TailLongSideLength() - 1.0f, 0, 1, 0 }; }
    inline winrt::Thickness BottomLeftPlacementTopRightHighlightMargin(double width, double height) { return { width - (MinimumTipEdgeToTailEdgeMargin() + 1.0f), 0, 1, 0 }; }
    static inline winrt::Thickness OtherPlacementTopRightHighlightMargin(double width, double height) { return { 0, 0, 0, 0 }; }

    inline winrt::Thickness BottomPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, (width / 2) + (TailShortSideLength() - 1.0f), 0 }; }
    inline winrt::Thickness BottomRightPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, width - (MinimumTipEdgeToTailEdgeMargin() + 1.0f), 0 }; }
    inline winrt::Thickness BottomLeftPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, MinimumTipEdgeToTailEdgeMargin() + TailLongSideLength() - 1.0f, 0 }; }
    static inline winrt::Thickness TopEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 1, 1, 1, 0 }; }
    // Shifted by one since the tail edge's border is not accounted for automatically.
    static inline winrt::Thickness LeftEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 1, 1, 0, 0 }; }
    static inline winrt::Thickness RightEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 0, 1, 1, 0 }; }

    static inline double UntargetedTipFarPlacementOffset(float windowSize, double tipSize, double offset) { return windowSize - (tipSize + s_untargetedTipWindowEdgeMargin + offset); }
    static inline double UntargetedTipCenterPlacementOffset(float windowSize, double tipSize, double nearOffset, double farOffset) { return (windowSize / 2) - (tipSize / 2) + nearOffset - farOffset; }
    static inline double UntargetedTipNearPlacementOffset(double offset) { return s_untargetedTipWindowEdgeMargin + offset; }

    static constexpr wstring_view s_scaleTargetName{ L"Scale"sv };
    static constexpr wstring_view s_translationTargetName{ L"Translation"sv };

    static constexpr wstring_view s_containerName{ L"Container"sv };
    static constexpr wstring_view s_popupName{ L"Popup"sv };
    static constexpr wstring_view s_tailOcclusionGridName{ L"TailOcclusionGrid"sv };
    static constexpr wstring_view s_contentRootGridName{ L"ContentRootGrid"sv };
    static constexpr wstring_view s_nonHeroContentRootGridName{ L"NonHeroContentRootGrid"sv };
    static constexpr wstring_view s_shadowTargetName{ L"ShadowTarget"sv };
    static constexpr wstring_view s_heroContentBorderName{ L"HeroContentBorder"sv };
    static constexpr wstring_view s_iconBorderName{ L"IconBorder"sv };
    static constexpr wstring_view s_titlesStackPanelName{ L"TitlesStackPanel"sv };
    static constexpr wstring_view s_titleTextBoxName{ L"TitleTextBlock"sv };
    static constexpr wstring_view s_subtitleTextBoxName{ L"SubtitleTextBlock"sv };
    static constexpr wstring_view s_alternateCloseButtonName{ L"AlternateCloseButton"sv };
    static constexpr wstring_view s_mainContentPresenterName{ L"MainContentPresenter"sv };
    static constexpr wstring_view s_actionButtonName{ L"ActionButton"sv };
    static constexpr wstring_view s_closeButtonName{ L"CloseButton"sv };
    static constexpr wstring_view s_tailPolygonName{ L"TailPolygon"sv };
    static constexpr wstring_view s_tailEdgeBorderName{ L"TailEdgeBorder"sv };
    static constexpr wstring_view s_topTailPolygonHighlightName{ L"TopTailPolygonHighlight"sv };
    static constexpr wstring_view s_topHighlightLeftName{ L"TopHighlightLeft"sv };
    static constexpr wstring_view s_topHighlightRightName{ L"TopHighlightRight"sv };

    static constexpr wstring_view s_accentButtonStyleName{ L"AccentButtonStyle" };
    static constexpr wstring_view s_teachingTipTopHighlightBrushName{ L"TeachingTipTopHighlightBrush" };

    static constexpr winrt::float2 s_expandAnimationEasingCurveControlPoint1{ 0.1f, 0.9f };
    static constexpr winrt::float2 s_expandAnimationEasingCurveControlPoint2{ 0.2f, 1.0f };
    static constexpr winrt::float2 s_contractAnimationEasingCurveControlPoint1{ 0.7f, 0.0f };
    static constexpr winrt::float2 s_contractAnimationEasingCurveControlPoint2{ 1.0f, 0.5f };

    //It is possible this should be exposed as a property, but you can adjust what it does with margin.
    static constexpr float s_untargetedTipWindowEdgeMargin = 24;
    static constexpr float s_defaultTipHeightAndWidth = 320;

    //Ideally this would be computed from layout but it is difficult to do.
    static constexpr float s_tailOcclusionAmount = 2;

    // The tail is designed as an 8x16 pixel shape, however it is actually a 10x20 shape which is partially occluded by the tip content.
    // This is done to get the border of the tip to follow the tail shape without drawing the border on the tip edge of the tail.
    inline float MinimumTipEdgeToTailEdgeMargin()
    {
        return m_tailOcclusionGrid.get().ColumnDefinitions().Size() > 1 ?
            static_cast<float>(m_tailOcclusionGrid.get().ColumnDefinitions().GetAt(1).ActualWidth() + s_tailOcclusionAmount)
            : 0.0f;
    }

    inline float MinimumTipEdgeToTailCenter()
    {
        return m_tailOcclusionGrid.get().ColumnDefinitions().Size() > 1 ?  
            static_cast<float>(m_tailOcclusionGrid.get().ColumnDefinitions().GetAt(0).ActualWidth() +
                m_tailOcclusionGrid.get().ColumnDefinitions().GetAt(1).ActualWidth() +
                (std::max(m_tailPolygon.get().ActualHeight(), m_tailPolygon.get().ActualWidth()) / 2))
            : 0.0f;
    }

    inline float TailLongSideActualLength() { return static_cast<float>(std::max(m_tailPolygon.get().ActualHeight(), m_tailPolygon.get().ActualWidth())); }
    inline float TailLongSideLength() { return static_cast<float>(TailLongSideActualLength() - (2 * s_tailOcclusionAmount)); }
    inline float TailShortSideLength() { return static_cast<float>(std::min(m_tailPolygon.get().ActualHeight(), m_tailPolygon.get().ActualWidth()) - s_tailOcclusionAmount); }
};
