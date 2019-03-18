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
    void SetPointerElevation(float elevation);
    bool GetIsIdle();
    winrt::TeachingTipPlacementMode GetEffectivePlacement();
    winrt::TeachingTipHeroContentPlacementMode GetEffectiveHeroContentPlacement();
    double GetHorizontalOffset();
    double GetVerticalOffset();
    void SetUseTestWindowBounds(bool useTestWindowBounds);
    void SetTestWindowBounds(const winrt::Rect& testWindowBounds);
    void SetTipFollowsTarget(bool tipFollowsTarget);
    void SetExpandAnimationDuration(const winrt::TimeSpan& expandAnimationDuration);
    void SetContractAnimationDuration(const winrt::TimeSpan& contractAnimationDuration);

private:
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
    winrt::Grid::Loaded_revoker m_pointerOcclusionGridLoadedRevoker{};
    void CreateLightDismissIndicatorPopup();
    void UpdatePointer();
    void PositionPopup();
    void PositionTargetedPopup();
    void PositionUntargetedPopup();
    void UpdateSizeBasedTemplateSettings();
    void UpdateButtonsState();
    void UpdateDynamicHeroContentPlacementToTop();
    void UpdateDynamicHeroContentPlacementToBottom();

    static void OnPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    void OnIsOpenChanged();
    void OnTargetChanged();
    void OnPointerModeChanged();
    void OnIconSourceChanged();
    void OnPlacementMarginChanged();
    void OnIsLightDismissEnabledChanged();
    void OnHeroContentPlacementChanged();

    void OnCloseButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void OnActionButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void OnPopupOpened(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnLightDismissIndicatorPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnPointerOcclusionGridLoaded(const winrt::IInspectable&, const winrt::IInspectable&);

    void RaiseClosingEvent();
    void ClosePopupWithAnimationIfAvailable();
    void ClosePopup();

    void SetViewportChangedEvent();
    void RevokeViewportChangedEvent();
    void TargetLayoutUpdated(const winrt::IInspectable&, const winrt::IInspectable&);

    void CreateExpandAnimation();
    void CreateContractAnimation();

    void StartExpandToOpen();
    void StartContractToClose();

    winrt::TeachingTipPlacementMode DetermineEffectivePlacement();
    void EstablishShadows();

    tracker_ref<winrt::Border> m_container{ this };

    tracker_ref<winrt::Popup> m_popup{ this };
    tracker_ref<winrt::Popup> m_lightDismissIndicatorPopup{ this };

    tracker_ref<winrt::UIElement> m_rootElement{ this };
    tracker_ref<winrt::Grid> m_pointerOcclusionGrid{ this };
    tracker_ref<winrt::Grid> m_contentRootGrid{ this };
    tracker_ref<winrt::Grid> m_nonHeroContentRootGrid{ this };
    tracker_ref<winrt::Border> m_heroContentBorder{ this };
    tracker_ref<winrt::Border> m_iconBorder{ this };
    tracker_ref<winrt::Button> m_actionButton{ this };
    tracker_ref<winrt::Button> m_alternateCloseButton{ this };
    tracker_ref<winrt::Button> m_closeButton{ this };
    tracker_ref<winrt::Polygon> m_pointerPolygon{ this };
    tracker_ref<winrt::Grid> m_pointerEdgeBorder{ this };

    tracker_ref<winrt::KeyFrameAnimation> m_expandAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_contractAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_expandElevationAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_contractElevationAnimation{ this };
    tracker_ref<winrt::CompositionEasingFunction> m_expandEasingFunction{ this };
    tracker_ref<winrt::CompositionEasingFunction> m_contractEasingFunction{ this };

    winrt::TeachingTipPlacementMode m_currentEffectiveTipPlacementMode{ winrt::TeachingTipPlacementMode::Auto };
    winrt::TeachingTipPlacementMode m_currentEffectivePointerPlacementMode{ winrt::TeachingTipPlacementMode::Auto };
    winrt::TeachingTipHeroContentPlacementMode m_currentHeroContentEffectivePlacementMode{ winrt::TeachingTipHeroContentPlacementMode::Auto };

    winrt::Rect m_currentBounds{ 0,0,0,0 };
    winrt::Rect m_currentTargetBounds{ 0,0,0,0 };

    bool m_isTemplateApplied{ false };

    bool m_isExpandAnimationPlaying{ false };
    bool m_isContractAnimationPlaying{ false };

    bool m_useTestWindowBounds{ false };
    winrt::Rect m_testWindowBounds{ 0,0,0,0 };

    bool m_tipShouldHaveShadow{ true };

    bool m_tipFollowsTarget{ false };

    float m_contentElevation{ 32.0f };
    float m_pointerElevation{ 0.0f };
    bool m_pointerShadowTargetsShadowTarget{ false };

    bool m_isIdle{ true };

    winrt::TimeSpan m_expandAnimationDuration{ 300ms };
    winrt::TimeSpan m_contractAnimationDuration{ 200ms };

    winrt::TeachingTipCloseReason m_lastCloseReason{ winrt::TeachingTipCloseReason::Programmatic };

    // These values are shifted by one because this is the 1px highlight that sits adjacent to the tip border.
    inline winrt::Thickness BottomPlacementTopRightHighlightMargin(double width, double height) { return { (width / 2) + (PointerShortSideLength() - 1.0f), 0, 1, 0 }; }
    inline winrt::Thickness BottomEdgeAlignedRightPlacementTopRightHighlightMargin(double width, double height) { return { MinimumTipEdgeToPointerEdgeMargin() + PointerLongSideLength() - 1.0f, 0, 1, 0 }; }
    inline winrt::Thickness BottomEdgeAlignedLeftPlacementTopRightHighlightMargin(double width, double height) { return { width - (MinimumTipEdgeToPointerEdgeMargin() + 1.0f), 0, 1, 0 }; }
    static inline winrt::Thickness OtherPlacementTopRightHighlightMargin(double width, double height) { return { 0, 0, 0, 0 }; }

    inline winrt::Thickness BottomPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, (width / 2) + (PointerShortSideLength() - 1.0f), 0 }; }
    inline winrt::Thickness BottomEdgeAlignedRightPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, width - (MinimumTipEdgeToPointerEdgeMargin() + 1.0f), 0 }; }
    inline winrt::Thickness BottomEdgeAlignedLeftPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, MinimumTipEdgeToPointerEdgeMargin() + PointerLongSideLength() - 1.0f, 0 }; }
    static inline winrt::Thickness TopEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 1, 1, 1, 0 }; }
    // Shifted by one since the pointer edge's border is not accounted for automatically.
    static inline winrt::Thickness LeftEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 1, 1, 0, 0 }; }
    static inline winrt::Thickness RightEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 0, 1, 1, 0 }; }

    static inline double UntargetedTipFarPlacementOffset(float windowSize, double tipSize, double offset) { return windowSize - (tipSize + s_untargetedTipWindowEdgeMargin + offset); }
    static inline double UntargetedTipCenterPlacementOffset(float windowSize, double tipSize, double nearOffset, double farOffset) { return (windowSize / 2) - (tipSize / 2) + nearOffset - farOffset; }
    static inline double UntargetedTipNearPlacementOffset(double offset) { return s_untargetedTipWindowEdgeMargin + offset; }

    static constexpr wstring_view s_scaleTargetName{ L"Scale"sv };
    static constexpr wstring_view s_translationTargetName{ L"Translation"sv };

    static constexpr wstring_view s_containerName{ L"Container"sv };
    static constexpr wstring_view s_popupName{ L"Popup"sv };
    static constexpr wstring_view s_pointerOcclusionGridName{ L"PointerOcclusionGrid"sv };
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
    static constexpr wstring_view s_pointerPolygonName{ L"PointerPolygon"sv };
    static constexpr wstring_view s_pointerEdgeBorderName{ L"PointerEdgeBorder"sv };
    static constexpr wstring_view s_topPointerPolygonHighlightName{ L"TopPointerPolygonHighlight"sv };
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
    static constexpr float s_pointerOcclusionAmount = 2;

    // The pointer is designed as an 8x16 pixel shape, however it is actually a 10x20 shape which is partially occluded by the tip content.
    // This is done to get the border of the tip to follow the pointer shape without drawing the border on the tip edge of the pointer.
    inline float MinimumTipEdgeToPointerEdgeMargin()
    {
        return m_pointerOcclusionGrid.get().ColumnDefinitions().Size() > 1 ?
            static_cast<float>(m_pointerOcclusionGrid.get().ColumnDefinitions().GetAt(1).ActualWidth() + s_pointerOcclusionAmount)
            : 0.0f;
    }

    inline float MinimumTipEdgeToPointerCenter()
    {
        return m_pointerOcclusionGrid.get().ColumnDefinitions().Size() > 1 ?  
            static_cast<float>(m_pointerOcclusionGrid.get().ColumnDefinitions().GetAt(0).ActualWidth() +
                m_pointerOcclusionGrid.get().ColumnDefinitions().GetAt(1).ActualWidth() +
                (std::max(m_pointerPolygon.get().ActualHeight(), m_pointerPolygon.get().ActualWidth()) / 2))
            : 0.0f;
    }

    inline float PointerLongSideActualLength() { return static_cast<float>(std::max(m_pointerPolygon.get().ActualHeight(), m_pointerPolygon.get().ActualWidth())); }
    inline float PointerLongSideLength() { return static_cast<float>(PointerLongSideActualLength() - (2 * s_pointerOcclusionAmount)); }
    inline float PointerShortSideLength() { return static_cast<float>(std::min(m_pointerPolygon.get().ActualHeight(), m_pointerPolygon.get().ActualWidth()) - s_pointerOcclusionAmount); }
};
