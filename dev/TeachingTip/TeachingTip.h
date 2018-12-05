#pragma once

#include "pch.h"
#include "common.h"

#include "TeachingTipTemplateSettings.h"

#include "TeachingTip.g.h"
#include "TeachingTip.properties.h"

using namespace std::chrono_literals;

class TeachingTip :
    public ReferenceTracker<TeachingTip, winrt::implementation::TeachingTipT>,
    public TeachingTipProperties
{

public:
    TeachingTip();

    // IFrameworkElement
    void OnApplyTemplate();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    //ContentControl
    void OnContentChanged(const winrt::IInspectable& oldContent, const winrt::IInspectable& newContent);

    //UIElement
    void OnBackgroundChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);
    winrt::AutomationPeer OnCreateAutomationPeer();

    tracker_ref<winrt::UIElement> m_target{ this };

    static void SetAttach(const winrt::UIElement& element, const winrt::TeachingTip& teachingTip);
    static winrt::TeachingTip GetAttach(const winrt::UIElement& element);

    //TestHooks
    void SetExpandEasingFunction(const winrt::CompositionEasingFunction& easingFunction);
    void SetContractEasingFunction(const winrt::CompositionEasingFunction& easingFunction);
    void SetContentElevation(const float elevation);
    void SetBeakElevation(const float elevation);
    void SetBeakShadowTargetsShadowTarget(const bool targetsShadowTarget);
    bool GetIsIdle();
    winrt::TeachingTipPlacementMode GetEffectivePlacement();
    winrt::TeachingTipBleedingImagePlacementMode GetEffectiveBleedingPlacement();
    double GetHorizontalOffset();
    double GetVerticalOffset();
    void SetUseTestWindowBounds(bool useTestWindowBounds);
    void SetTestWindowBounds(const winrt::Rect& testWindowBounds);

private:
    winrt::FrameworkElement::SizeChanged_revoker m_ContentSizeChangedRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_targetLayoutUpdatedRevoker{};
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

    void OnXCloseButtonClicked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnCloseButtonClicked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnActionButtonClicked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnPopupClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    void RaiseClosingEvent(const winrt::TeachingTipCloseReason& reason);
    void ClosePopupWithAnimation();
    void ClosePopup();

    void SetBackgroundToDefault();

    void TargetLayoutUpdated();

    void CreateExpandAnimation();
    void CreateContractAnimation();

    void StartExpandAnimation();
    void StartContractAnimation();

    winrt::TeachingTipPlacementMode DetermineEffectivePlacement();
    void EstablishShadows();

    tracker_ref<winrt::Popup> m_popup{ this };

    tracker_ref<winrt::Grid> m_rootGrid{ this };
    tracker_ref<winrt::Grid> m_nonBleedingContentRootGrid{ this };
    tracker_ref<winrt::Grid> m_shadowTarget{ this };
    tracker_ref<winrt::ContentPresenter> m_bleedingImageContentPresenter{ this };
    tracker_ref<winrt::ContentPresenter> m_iconContentPresenter{ this };
    tracker_ref<winrt::Button> m_actionButton{ this };
    tracker_ref<winrt::Button> m_xCloseButton{ this };
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
    bool m_hasCustomBackground{ false };
    bool m_haveSetDefaultBackground{ false };

    bool m_isIdle{ true };

    winrt::TeachingTipCloseReason m_lastCloseReason{ winrt::TeachingTipCloseReason::Programmatic };

    static inline winrt::Thickness TopBeakMargin(const double width, const double height) { return { (width / 2) - 10, height - 3, 0, 0 }; }
    static inline winrt::Thickness BottomBeakMargin(const double width, const double height) { return { (width / 2) - 10, 0, 0, 0 }; }
    static inline winrt::Thickness LeftBeakMargin(const double width, const double height) { return { width - 2, (height / 2) - 10, 0, 0 }; }
    static inline winrt::Thickness RightBeakMargin(const double width, const double height) { return { 0, (height / 2) - 10, 0, 0 }; }
    static inline winrt::Thickness TopEdgeAlignedRightBeakMargin(const double width, const double height) { return { 10, height - 3, 0, 0 }; }
    static inline winrt::Thickness TopEdgeAlignedLeftBeakMargin(const double width, const double height) { return { width - 30, height - 3, 0, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedRightBeakMargin(const double width, const double height) { return { 10, 0, 0, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedLeftBeakMargin(const double width, const double height) { return { width - 30, 0, 0, 0 }; }
    static inline winrt::Thickness LeftEdgeAlignedTopBeakMargin(const double width, const double height) { return { width - 2, height - 30, 0, 0 }; }
    static inline winrt::Thickness LeftEdgeAlignedBottomBeakMargin(const double width, const double height) { return { width - 2, 10, 0, 0 }; }
    static inline winrt::Thickness RightEdgeAlignedTopBeakMargin(const double width, const double height) { return { 0, height - 30, 0, 0 }; }
    static inline winrt::Thickness RightEdgeAlignedBottomBeakMargin(const double width, const double height) { return { 0, 10, 0, 0 }; }
    static inline winrt::Thickness OtherBeakMargin(const double width, const double height) { return { 0, 0, 0, 0 }; }

    static inline winrt::Thickness BottomBeakHightlightMargin(const double width, const double height) { return { (width / 2) - 11, -7, 0, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedRightBeakHighlightMargin(const double width, const double height) { return { 9, -7, 0, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedLeftBeakHighlightMargin(const double width, const double height) { return { width - 31, -7, 0, 0 }; }
    static inline winrt::Thickness OtherBeakHighlightMargin(const double width, const double height) { return { 0, 0, 0, 0 }; }

    static inline winrt::Thickness BottomPlacementTopRightHightMargin(const double width, const double height) { return { (width / 2) + 7, 1, 0, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedRightPlacementTopRightHighlightMargin(const double width, const double height) { return { 27, 1, 0, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedLeftPlacementTopRightHighlightMargin(const double width, const double height) { return { width - 13, 1, 0, 0 }; }
    static inline winrt::Thickness OtherPlacementTopRightHighlightMargin(const double width, const double height) { return { 0, 0, 0, 0 }; }

    static inline winrt::Thickness BottomPlacementTopLeftHightMargin(const double width, const double height) { return { 0, 1, (width / 2) + 7, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedRightPlacementTopLeftHighlightMargin(const double width, const double height) { return { 0, 1, width - 13, 0 }; }
    static inline winrt::Thickness BottomEdgeAlignedLeftPlacementTopLeftHighlightMargin(const double width, const double height) { return { 0, 1, 27, 0 }; }
    static inline winrt::Thickness TopEdgePlacementTopLeftHighlightMargin(const double width, const double height) { return { 0, 0, 0, 0 }; }
    static inline winrt::Thickness LeftEdgePlacementTopLeftHighlightMargin(const double width, const double height) { return { 0, 0, 1, 0 }; }
    static inline winrt::Thickness RightEdgePlacementTopLeftHighlightMargin(const double width, const double height) { return { 1, 0, 0, 0 }; }

    static inline double UntargettedTipFarPlacementOffset(const float windowSize, const double tipSize, const double offset) { return windowSize - (tipSize + s_untargetedTipWindowEdgeMargin + offset); }
    static inline double UntargettedTipCenterPlacementOffset(const float windowSize, const double tipSize, const double nearOffset, const double farOffset) { return (windowSize / 2) - (tipSize / 2) + nearOffset - farOffset; }
    static inline double UntargettedTipNearPlacementOffset(const double offset) { return s_untargetedTipWindowEdgeMargin + offset; }

    static constexpr inline bool isPlacementBottom(const winrt::TeachingTipPlacementMode& placement)
    {
        return placement == winrt::TeachingTipPlacementMode::Bottom ||
            placement == winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft ||
            placement == winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight;
    }

    static constexpr inline bool isPlacementTop(const winrt::TeachingTipPlacementMode& placement)
    {
        return placement == winrt::TeachingTipPlacementMode::Top ||
            placement == winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft ||
            placement == winrt::TeachingTipPlacementMode::TopEdgeAlignedRight;
    }

    static constexpr inline bool isPlacementRight(const winrt::TeachingTipPlacementMode& placement)
    {
        return placement == winrt::TeachingTipPlacementMode::Right ||
            placement == winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom ||
            placement == winrt::TeachingTipPlacementMode::RightEdgeAlignedTop;
    }

    static constexpr inline bool isPlacementLeft(const winrt::TeachingTipPlacementMode& placement)
    {
        return placement == winrt::TeachingTipPlacementMode::Left ||
            placement == winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom ||
            placement == winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop;
    }

    static constexpr winrt::TimeSpan s_expandAnimationDuration{ 300ms };
    static constexpr winrt::TimeSpan s_contractAnimationDuration{ 200ms };

    static constexpr wstring_view s_scaleTargetName{ L"Scale"sv };
    static constexpr wstring_view s_translationTargetName{ L"Translation"sv };

    static constexpr wstring_view s_rootGridName{ L"RootGrid"sv };
    static constexpr wstring_view s_contentRootGridName{ L"ContentRootGrid"sv };
    static constexpr wstring_view s_nonBleedingContentRootGridName{ L"NonBleedingContentRootGrid"sv };
    static constexpr wstring_view s_shadowTargetName{ L"ShadowTarget"sv };
    static constexpr wstring_view s_bleedingImageContentPresenterName{ L"BleedingImageContentPresenter"sv };
    static constexpr wstring_view s_iconName{ L"IconContentPresenter"sv };
    static constexpr wstring_view s_titlesStackPanelName{ L"TitlesStackPanel"sv };
    static constexpr wstring_view s_titleTextBoxName{ L"TitleTextBlock"sv };
    static constexpr wstring_view s_subtextTextBoxName{ L"SubtextTextBlock"sv };
    static constexpr wstring_view s_xCloseButtonName{ L"XCloseButton"sv };
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
    static constexpr wstring_view s_teachingTipTransientBackgroundBrushName{ L"TeachingTipTransientBackgroundBrush" };
    static constexpr wstring_view s_teachingTipStaticBackgroundBrushName{ L"TeachingTipStaticBackgroundBrush" };

    static constexpr winrt::float2 s_expandAnimationEasingCurveControlPoint1{ 0.1f, 0.9f };
    static constexpr winrt::float2 s_expandAnimationEasingCurveControlPoint2{ 0.2f, 1.0f };
    static constexpr winrt::float2 s_contractAnimationEasingCurveControlPoint1{ 0.7f, 0.0f };
    static constexpr winrt::float2 s_contractAnimationEasingCurveControlPoint2{ 1.0f, 0.5f };

    static constexpr float s_untargetedTipWindowEdgeMargin = 24;

    static constexpr float s_defaultTipHeightAndWidth = 320;

    //The beak is designed as an 8x16 pixel shape, however it is actual a 10x20 shape which is partially occulded by the tip content.
    //This is done to get the border of the tip to follow the beak shape without drawing the border on the tip edge of the beak.
    static constexpr float s_minimumTipEdgeToBeakEdgeMargin = 12;
    static constexpr float s_minimumActualTipEdgeToBeakEdgeMargin = 10;

    static constexpr float s_minimumTipEdgeToBeakCenter = 20;

    static constexpr float s_beakLongSideLength = 16;
    static constexpr float s_beakLongSideActualLength = 20;
    static constexpr float s_beakShortSideLength = 8;
    static constexpr float s_beakShortSideActualLength = 10;
};
