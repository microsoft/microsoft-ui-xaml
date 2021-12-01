// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwipeControlTrace.h"
#include "SwipeControl.g.h"

#include "SwipeControl.properties.h"

enum class CreatedContent { Left, Top, Bottom, Right, None };

class SwipeControl :
    public ReferenceTracker<SwipeControl, winrt::implementation::SwipeControlT>,
    public SwipeControlProperties
{
public:
    SwipeControl();
    virtual ~SwipeControl();

#pragma region ISwipeControl

    void Close();

#pragma endregion

#pragma region IFrameworkElementOverrides
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
#pragma endregion

    void CustomAnimationStateEntered(
        winrt::InteractionTrackerCustomAnimationStateEnteredArgs const& args);

    void RequestIgnored(
        winrt::InteractionTrackerRequestIgnoredArgs const& args);

    void IdleStateEntered(
        winrt::InteractionTrackerIdleStateEnteredArgs const& args);

    void InteractingStateEntered(
        winrt::InteractionTrackerInteractingStateEnteredArgs const& args);

    void InertiaStateEntered(
        winrt::InteractionTrackerInertiaStateEnteredArgs const& args);

    void ValuesChanged(
        winrt::InteractionTrackerValuesChangedArgs const& args);

    winrt::SwipeItems GetCurrentItems() { return m_currentItems.get(); }

#pragma region TestHookHelpers
    static winrt::SwipeControl GetLastInteractedWithSwipeControl();
    bool GetIsOpen();
    bool GetIsIdle();
#pragma endregion

private:
    void OnLeftItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/);
    void OnRightItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/);
    void OnBottomItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/);
    void OnTopItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/);
    void OnLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/);

    void AttachEventHandlers();
    void DetachEventHandlers(bool useSafeGet);
    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnSwipeContentStackPanelSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnPointerPressedEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void InputEaterGridTapped(const winrt::IInspectable& /*sender*/, const winrt::TappedRoutedEventArgs& args);

    void AttachDismissingHandlers();
    void DetachDismissingHandlers();
    void DismissSwipeOnAcceleratorKeyActivator(const winrt::Windows::UI::Core::CoreDispatcher & sender, const winrt::AcceleratorKeyEventArgs & args);

    // Used on platforms where we have XamlRoot.
    void CurrentXamlRootChanged(const winrt::XamlRoot & sender, const winrt::XamlRootChangedEventArgs & args);
    
    // Used on platforms where we don't have XamlRoot.
    void DismissSwipeOnCoreWindowKeyDown(const winrt::CoreWindow & sender, const winrt::KeyEventArgs & args);
    void CurrentWindowSizeChanged(const winrt::IInspectable & sender, const winrt::WindowSizeChangedEventArgs& args);
    void CurrentWindowVisibilityChanged(const winrt::CoreWindow & sender, const winrt::VisibilityChangedEventArgs args);
    void DismissSwipeOnAnExternalCoreWindowTap(const winrt::CoreWindow& sender, const winrt::PointerEventArgs& args);

    void DismissSwipeOnAnExternalTap(winrt::Point const& tapPoint);

    void GetTemplateParts();

    void InitializeInteractionTracker();
    void ConfigurePositionInertiaRestingValues();

    winrt::Visual FindVisualInteractionSourceVisual();
    void EnsureClip();

    void CloseWithoutAnimation();
    void CloseIfNotRemainOpenExecuteItem();

    void CreateLeftContent();
    void CreateRightContent();
    void CreateTopContent();
    void CreateBottomContent();
    void CreateContent(const winrt::SwipeItems& items);

    void AlignStackPanel();
    void PopulateContentItems();
    void SetupExecuteExpressionAnimation();
    void SetupClipAnimation();
    void UpdateColors();

    winrt::AppBarButton GetSwipeItemButton(const winrt::SwipeItem& swipeItem);
    void UpdateColorsIfExecuteItem();
    void UpdateColorsIfRevealItems();
    void UpdateExecuteForegroundColor(const winrt::SwipeItem& swipeItem);
    void UpdateExecuteBackgroundColor(const winrt::SwipeItem& swipeItem);

    void OnLeftItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args);
    void OnRightItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args);
    void OnTopItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args);
    void OnBottomItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args);

    void TryGetSwipeVisuals();
    void UpdateIsOpen(bool isOpen);
    void UpdateThresholdReached(float value);

    void ThrowIfHasVerticalAndHorizontalContent(bool IsHorizontal = false);

    std::wstring GetAnimationTarget(winrt::UIElement child);

    winrt::SwipeControl GetThis();

    winrt::IInteractionTrackerOwner m_interactionTrackerOwner{ nullptr };

    tracker_ref<winrt::Grid> m_rootGrid{ this };
    tracker_ref<winrt::Grid> m_content{ this };
    tracker_ref<winrt::Grid> m_inputEater{ this };
    tracker_ref<winrt::Grid> m_swipeContentRoot{ this };
    tracker_ref<winrt::StackPanel> m_swipeContentStackPanel{ this };

    tracker_ref<winrt::InteractionTracker> m_interactionTracker{ this };
    tracker_ref<winrt::VisualInteractionSource> m_visualInteractionSource{ this };
    tracker_ref<winrt::Compositor> m_compositor{ this };

    tracker_ref<winrt::Visual> m_mainContentVisual{ this };
    tracker_ref<winrt::Visual> m_swipeContentRootVisual{ this };
    tracker_ref<winrt::Visual> m_swipeContentVisual{ this };
    tracker_ref<winrt::InsetClip> m_insetClip{ this };

    tracker_ref<winrt::ExpressionAnimation> m_swipeAnimation{ this };
    tracker_ref<winrt::ExpressionAnimation> m_executeExpressionAnimation{ this };
    tracker_ref<winrt::ExpressionAnimation> m_clipExpressionAnimation{ this };
    tracker_ref<winrt::ExpressionAnimation> m_maxPositionExpressionAnimation{ this };
    tracker_ref<winrt::ExpressionAnimation> m_minPositionExpressionAnimation{ this };

    tracker_ref<winrt::Style> m_swipeItemStyle{ this };

    // Cache the current content object to minimize work if there are multiple swipes in the same direction.
    tracker_ref<winrt::SwipeItems> m_currentItems{ this };

    winrt::event_token m_loadedToken{};
    winrt::event_token m_leftItemsChangedToken{};
    winrt::event_token m_rightItemsChangedToken{};
    winrt::event_token m_topItemsChangedToken{};
    winrt::event_token m_bottomItemsChangedToken{};
    winrt::event_token m_onSizeChangedToken{};
    winrt::event_token m_onSwipeContentStackPanelSizeChangedToken{};
    winrt::event_token m_inputEaterTappedToken{};
    tracker_ref<winrt::IInspectable> m_onPointerPressedEventHandler{ this };

    // Used on platforms where we have XamlRoot.
    RoutedEventHandler_revoker m_xamlRootPointerPressedEventRevoker{};
    RoutedEventHandler_revoker m_xamlRootKeyDownEventRevoker{};
    XamlRootChanged_revoker m_xamlRootChangedRevoker{};

    // Used on platforms where we don't have XamlRoot.
    winrt::ICoreWindow::PointerPressed_revoker m_coreWindowPointerPressedRevoker;
    winrt::ICoreWindow::KeyDown_revoker m_coreWindowKeyDownRevoker;
    winrt::ICoreWindow::VisibilityChanged_revoker m_windowMinimizeRevoker;
    winrt::IWindow::SizeChanged_revoker m_windowSizeChangedRevoker;

    winrt::CoreAcceleratorKeys::AcceleratorKeyActivated_revoker m_acceleratorKeyActivatedRevoker;

    bool m_hasInitialLoadedEventFired{ false };

    bool m_lastActionWasClosing{ false };
    bool m_lastActionWasOpening{ false };
    bool m_isInteracting{ false };
    bool m_isIdle{ true };
    bool m_isOpen{ false };
    bool m_thresholdReached{ false };
    //Near content = left or top
    //Far content = right or bottom
    bool m_blockNearContent{ false };
    bool m_blockFarContent{ false };
    bool m_isHorizontal{ true };
    CreatedContent m_createdContent{ CreatedContent::None };

    static bool IsTranslationFacadeAvailableForSwipeControl(const winrt::UIElement& element);
    static wstring_view DirectionToInset(const CreatedContent& createdContent);

    static constexpr wstring_view s_isNearOpenPropertyName{ L"isNearOpen"sv };
    static inline const std::wstring isNearOpenPropertyName() { return s_isNearOpenPropertyName.data(); }
    static constexpr wstring_view s_isFarOpenPropertyName{ L"isFarOpen"sv };
    static inline const std::wstring isFarOpenPropertyName() { return s_isFarOpenPropertyName.data(); }
    static constexpr wstring_view s_isNearContentPropertyName{ L"isNearContent"sv };
    static inline const std::wstring isNearContentPropertyName() { return s_isNearContentPropertyName.data(); }
    static constexpr wstring_view s_blockNearContentPropertyName{ L"blockNearContent"sv };
    static inline const std::wstring blockNearContentPropertyName() { return s_blockNearContentPropertyName.data(); }
    static constexpr wstring_view s_blockFarContentPropertyName{ L"blockFarContent"sv };
    static inline const std::wstring blockFarContentPropertyName() { return s_blockFarContentPropertyName.data(); }

    static constexpr wstring_view s_hasLeftContentPropertyName{ L"hasLeftContent"sv };
    static inline const std::wstring hasLeftContentPropertyName() { return s_hasLeftContentPropertyName.data(); }
    static constexpr wstring_view s_hasRightContentPropertyName{ L"hasRightContent"sv };
    static inline const std::wstring hasRightContentPropertyName() { return s_hasRightContentPropertyName.data(); }
    static constexpr wstring_view s_hasTopContentPropertyName{ L"hasTopContent"sv };
    static inline const std::wstring hasTopContentPropertyName() { return s_hasTopContentPropertyName.data(); }
    static constexpr wstring_view s_hasBottomContentPropertyName{ L"hasBottomContent"sv };
    static inline const std::wstring hasBottomContentPropertyName() { return s_hasBottomContentPropertyName.data(); }
    static constexpr wstring_view s_isHorizontalPropertyName{ L"isHorizontal"sv };
    static inline const std::wstring isHorizontalPropertyName() { return s_isHorizontalPropertyName.data(); }

    static constexpr wstring_view s_trackerPropertyName{ L"tracker"sv };
    static inline const std::wstring trackerPropertyName() { return s_trackerPropertyName.data(); }
    static constexpr wstring_view s_foregroundVisualPropertyName{ L"foregroundVisual"sv };
    static inline const std::wstring foregroundVisualPropertyName() { return s_foregroundVisualPropertyName.data(); }
    static constexpr wstring_view s_swipeContentVisualPropertyName{ L"swipeContentVisual"sv };
    static inline const std::wstring swipeContentVisualPropertyName() { return s_swipeContentVisualPropertyName.data(); }
    static constexpr wstring_view s_swipeContentSizeParameterName{ L"swipeContentVisual"sv };
    static inline const std::wstring swipeContentSizeParameterName() { return s_swipeContentSizeParameterName.data(); }
    static constexpr wstring_view s_swipeRootVisualPropertyName{ L"swipeRootVisual"sv };
    static inline const std::wstring swipeRootVisualPropertyName() { return s_swipeRootVisualPropertyName.data(); }
    static constexpr wstring_view s_maxThresholdPropertyName{ L"maxThreshold"sv };
    static inline const std::wstring maxThresholdPropertyName() { return s_maxThresholdPropertyName.data(); }

    static constexpr wstring_view s_minPositionPropertyName{ L"minPosition"sv };
    static constexpr wstring_view s_maxPositionPropertyName{ L"maxPosition"sv };

    static constexpr wstring_view s_leftInsetTargetName{ L"LeftInset"sv };
    static constexpr wstring_view s_rightInsetTargetName{ L"RightInset"sv };
    static constexpr wstring_view s_topInsetTargetName{ L"TopInset"sv };
    static constexpr wstring_view s_bottomInsetTargetName{ L"BottomInset"sv };

    static constexpr wstring_view s_translationPropertyName{ L"Translation"sv };
    static constexpr wstring_view s_offsetPropertyName{ L"Offset"sv };

    static constexpr wstring_view s_rootGridName{ L"RootGrid"sv };
    static constexpr wstring_view s_inputEaterName{ L"InputEater"sv };
    static constexpr wstring_view s_ContentRootName{ L"ContentRoot"sv };
    static constexpr wstring_view s_swipeContentRootName{ L"SwipeContentRoot"sv };
    static constexpr wstring_view s_swipeContentStackPanelName{ L"SwipeContentStackPanel"sv };
    static constexpr wstring_view s_swipeItemStyleName{ L"SwipeItemStyle"sv };


    static constexpr wstring_view s_swipeItemBackgroundResourceName{ L"SwipeItemBackground"sv };
    static constexpr wstring_view s_swipeItemForegroundResourceName{ L"SwipeItemForeground"sv };
    static constexpr wstring_view s_executeSwipeItemPreThresholdBackgroundResourceName{ L"SwipeItemPreThresholdExecuteBackground"sv };
    static constexpr wstring_view s_executeSwipeItemPostThresholdBackgroundResourceName{ L"SwipeItemPostThresholdExecuteBackground"sv };
    static constexpr wstring_view s_executeSwipeItemPreThresholdForegroundResourceName{ L"SwipeItemPreThresholdExecuteForeground"sv };
    static constexpr wstring_view s_executeSwipeItemPostThresholdForegroundResourceName{ L"SwipeItemPostThresholdExecuteForeground"sv };
};
