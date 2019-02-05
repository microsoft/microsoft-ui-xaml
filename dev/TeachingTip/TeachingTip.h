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

    tracker_ref<winrt::UIElement> m_target{ this };

    static void SetAttach(const winrt::UIElement& element, const winrt::TeachingTip& teachingTip);
    static winrt::TeachingTip GetAttach(const winrt::UIElement& element);

    // TestHooks
    void SetExpandEasingFunction(const winrt::CompositionEasingFunction& easingFunction);
    void SetContractEasingFunction(const winrt::CompositionEasingFunction& easingFunction);
    void SetContentElevation(float elevation);
    void SetBeakElevation(float elevation);
    void SetBeakShadowTargetsShadowTarget(bool targetsShadowTarget);
    bool GetIsIdle();
    winrt::TeachingTipPlacementMode GetEffectivePlacement();
    winrt::TeachingTipBleedingImagePlacementMode GetEffectiveBleedingPlacement();
    double GetHorizontalOffset();
    double GetVerticalOffset();
    void SetUseTestWindowBounds(bool useTestWindowBounds);
    void SetTestWindowBounds(const winrt::Rect& testWindowBounds);

private:
    winrt::Button::Click_revoker m_closeButtonClickedRevoker{};
    winrt::Button::Click_revoker m_alternateCloseButtonClickedRevoker{};
    winrt::Button::Click_revoker m_actionButtonClickedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_contentSizeChangedRevoker{};
    winrt::FrameworkElement::EffectiveViewportChanged_revoker m_targetEffectiveViewportChangedRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_targetLayoutUpdatedRevoker{};
    winrt::Popup::Closed_revoker m_popupClosedRevoker{};
    winrt::Window::SizeChanged_revoker m_windowSizeChangedRevoker{};
    void UpdateBeak();
    void PositionPopup();
    void PositionTargetedPopup();
    void PositionUntargetedPopup();
    void UpdateSizeBasedTemplateSettings();
    void UpdateButtonsState();
    void UpdateDynamicBleedingContentPlacementToTop();
    void UpdateDynamicBleedingContentPlacementToBottom();

    static void OnPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    static void OnAttachPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    void OnIsOpenChanged();
    void OnIconSourceChanged();
    void OnTargetOffsetChanged();
    void OnIsLightDismissEnabledChanged();
    void OnBleedingImagePlacementChanged();

    void OnCloseButtonClicked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnActionButtonClicked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnPopupClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    void RaiseClosingEvent();
    void ClosePopupWithAnimationIfAvailable();
    void ClosePopup();

    void SetTarget(const winrt::UIElement& element);
    void SetViewportChangedEvent();
    void RevokeViewportChangedEvent();
    void TargetLayoutUpdated();

    void CreateExpandAnimation();
    void CreateContractAnimation();

    void StartExpandToOpen();
    void StartContractToClose();

    winrt::TeachingTipPlacementMode DetermineEffectivePlacement();
    void EstablishShadows();

    tracker_ref<winrt::Popup> m_popup{ this };

    tracker_ref<winrt::Grid> m_beakOcclusionGrid{ this };
    tracker_ref<winrt::Grid> m_contentRootGrid{ this };
    tracker_ref<winrt::Grid> m_nonBleedingContentRootGrid{ this };
    tracker_ref<winrt::Grid> m_shadowTarget{ this };
    tracker_ref<winrt::ContentPresenter> m_bleedingImageContentPresenter{ this };
    tracker_ref<winrt::ContentPresenter> m_iconContentPresenter{ this };
    tracker_ref<winrt::Button> m_actionButton{ this };
    tracker_ref<winrt::Button> m_alternateCloseButton{ this };
    tracker_ref<winrt::Button> m_closeButton{ this };
    tracker_ref<winrt::Polygon> m_beakPolygon{ this };
    tracker_ref<winrt::Grid> m_beakEdgeBorder{ this };

    tracker_ref<winrt::KeyFrameAnimation> m_expandAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_contractAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_expandElevationAnimation{ this };
    tracker_ref<winrt::KeyFrameAnimation> m_contractElevationAnimation{ this };
    tracker_ref<winrt::CompositionEasingFunction> m_expandEasingFunction{ this };
    tracker_ref<winrt::CompositionEasingFunction> m_contractEasingFunction{ this };

    winrt::TeachingTipPlacementMode m_currentEffectivePlacementMode{ winrt::TeachingTipPlacementMode::Auto };
    winrt::TeachingTipBleedingImagePlacementMode m_currentBleedingEffectivePlacementMode{ winrt::TeachingTipBleedingImagePlacementMode::Auto };

    winrt::Rect m_currentTargetBounds{ 0,0,0,0 };

    bool m_isExpandAnimationPlaying{ false };
    bool m_isContractAnimationPlaying{ false };

    bool m_useTestWindowBounds{ false };
    winrt::Rect m_testWindowBounds{ 0,0,0,0 };

    float m_contentElevation{ 32.0f };
    float m_beakElevation{ 0.0f };
    bool m_beakShadowTargetsShadowTarget{ false };

    bool m_startAnimationInOnApplyTemplate{ false };

    bool m_isIdle{ true };

    winrt::TeachingTipCloseReason m_lastCloseReason{ winrt::TeachingTipCloseReason::Programmatic };

    // These values are shifted by one because this is the 1px highlight that sits adjacent to the tip border.
    inline winrt::Thickness BottomPlacementTopRightHighlightMargin(double width, double height) { return { (width / 2) + (BeakShortSideLength() - 1), 0, 1, 0 }; }
    inline winrt::Thickness BottomEdgeAlignedRightPlacementTopRightHighlightMargin(double width, double height) { return { MinimumTipEdgeToBeakEdgeMargin() + BeakLongSideLength() - 1, 0, 1, 0 }; }
    inline winrt::Thickness BottomEdgeAlignedLeftPlacementTopRightHighlightMargin(double width, double height) { return { width - (MinimumTipEdgeToBeakEdgeMargin() + 1), 0, 1, 0 }; }
    static inline winrt::Thickness OtherPlacementTopRightHighlightMargin(double width, double height) { return { 0, 0, 0, 0 }; }

    inline winrt::Thickness BottomPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, (width / 2) + (BeakShortSideLength() - 1), 0 }; }
    inline winrt::Thickness BottomEdgeAlignedRightPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, width - (MinimumTipEdgeToBeakEdgeMargin() + 1), 0 }; }
    inline winrt::Thickness BottomEdgeAlignedLeftPlacementTopLeftHighlightMargin(double width, double height) { return { 1, 0, MinimumTipEdgeToBeakEdgeMargin() + BeakLongSideLength() - 1, 0 }; }
    static inline winrt::Thickness TopEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 1, 1, 1, 0 }; }
    // Shifted by one since the beak edge's border is not accounted for automatically.
    static inline winrt::Thickness LeftEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 1, 1, 0, 0 }; }
    static inline winrt::Thickness RightEdgePlacementTopLeftHighlightMargin(double width, double height) { return { 0, 1, 1, 0 }; }

    static inline double UntargetedTipFarPlacementOffset(float windowSize, double tipSize, double offset) { return windowSize - (tipSize + s_untargetedTipWindowEdgeMargin + offset); }
    static inline double UntargetedTipCenterPlacementOffset(float windowSize, double tipSize, double nearOffset, double farOffset) { return (windowSize / 2) - (tipSize / 2) + nearOffset - farOffset; }
    static inline double UntargetedTipNearPlacementOffset(double offset) { return s_untargetedTipWindowEdgeMargin + offset; }

    static constexpr winrt::TimeSpan s_expandAnimationDuration{ 300ms };
    static constexpr winrt::TimeSpan s_contractAnimationDuration{ 200ms };

    static constexpr wstring_view s_scaleTargetName{ L"Scale"sv };
    static constexpr wstring_view s_translationTargetName{ L"Translation"sv };

    static constexpr wstring_view s_beakOcclusionGridName{ L"BeakOcclusionGrid"sv };
    static constexpr wstring_view s_contentRootGridName{ L"ContentRootGrid"sv };
    static constexpr wstring_view s_nonBleedingContentRootGridName{ L"NonBleedingContentRootGrid"sv };
    static constexpr wstring_view s_shadowTargetName{ L"ShadowTarget"sv };
    static constexpr wstring_view s_bleedingImageContentPresenterName{ L"BleedingImageContentPresenter"sv };
    static constexpr wstring_view s_iconName{ L"IconContentPresenter"sv };
    static constexpr wstring_view s_titlesStackPanelName{ L"TitlesStackPanel"sv };
    static constexpr wstring_view s_titleTextBoxName{ L"TitleTextBlock"sv };
    static constexpr wstring_view s_subtextTextBoxName{ L"SubtextTextBlock"sv };
    static constexpr wstring_view s_alternateCloseButtonName{ L"AlternateCloseButton"sv };
    static constexpr wstring_view s_mainContentPresenterName{ L"MainContentPresenter"sv };
    static constexpr wstring_view s_actionButtonName{ L"ActionButton"sv };
    static constexpr wstring_view s_closeButtonName{ L"CloseButton"sv };
    static constexpr wstring_view s_beakPolygonName{ L"BeakPolygon"sv };
    static constexpr wstring_view s_beakEdgeBorderName{ L"BeakEdgeBorder"sv };
    static constexpr wstring_view s_topBeakPolygonHighlightName{ L"TopBeakPolygonHighlight"sv };
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

    //Ideally this would be computed from playout but it is difficult to do.
    static constexpr float s_beakOcclusionAmount = 2;

    // The beak is designed as an 8x16 pixel shape, however it is actual a 10x20 shape which is partially occluded by the tip content.
    // This is done to get the border of the tip to follow the beak shape without drawing the border on the tip edge of the beak.
    inline float MinimumTipEdgeToBeakEdgeMargin() { return static_cast<float>(m_beakOcclusionGrid.get().ColumnDefinitions().GetAt(1).ActualWidth() + s_beakOcclusionAmount); };
    inline float MinimumTipEdgeToBeakCenter() { return static_cast<float>(m_beakOcclusionGrid.get().ColumnDefinitions().GetAt(0).ActualWidth() +
                                                m_beakOcclusionGrid.get().ColumnDefinitions().GetAt(1).ActualWidth() +
                                                (std::max(m_beakPolygon.get().ActualHeight(), m_beakPolygon.get().ActualWidth()) / 2)); }

    inline float BeakLongSideActualLength() { return static_cast<float>(std::max(m_beakPolygon.get().ActualHeight(), m_beakPolygon.get().ActualWidth())); }
    inline float BeakLongSideLength() { return static_cast<float>(BeakLongSideActualLength() - (2 * s_beakOcclusionAmount)); }
    inline float BeakShortSideLength() { return static_cast<float>(std::min(m_beakPolygon.get().ActualHeight(), m_beakPolygon.get().ActualWidth()) - s_beakOcclusionAmount); }
};
