// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollInputHelper.h"

#include "ParallaxView.g.h"
#include "ParallaxView.properties.h"

class ParallaxView :
    public ReferenceTracker<ParallaxView, DeriveFromPanelHelper_base, winrt::ParallaxView>,
    public ParallaxViewProperties
{
public:
    ParallaxView();
    ~ParallaxView();

#pragma region IFrameworkElementOverridesHelper
    // IFrameworkElementOverrides (unoverridden methods provided by FrameworkElementOverridesHelper)
    winrt::Size MeasureOverride(winrt::Size const& availableSize); // not actually final for 'derived' classes
    winrt::Size ArrangeOverride(winrt::Size const& finalSize); // not actually final for 'derived' classes
#pragma endregion


    void RefreshAutomaticHorizontalOffsets();
    void RefreshAutomaticVerticalOffsets();

    // Invoked when a dependency property of this ParallaxView has changed.
    void OnPropertyChanged(
        const winrt::DependencyPropertyChangedEventArgs& args);

    // Invoked by ScrollInputHelper when a characteristic changes requires a re-evaluation of the parallaxing expression animations.
    void OnScrollInputHelperInfoChanged(
        bool horizontalInfoChanged, bool verticalInfoChanged);

private:
    static bool IsVisualTranslationPropertyAvailable();
    static wstring_view GetVisualTargetedPropertyName(winrt::Orientation orientation);

    void EnsureAnimatedVariables();
    void UpdateStartOffsetExpression(winrt::Orientation orientation);
    void UpdateEndOffsetExpression(winrt::Orientation orientation);
    void UpdateExpressionAnimation(winrt::Orientation orientation);
    void UpdateChild(const winrt::UIElement& oldChild, const winrt::UIElement& newChild);

    void HookLoaded();
    void HookSizeChanged();
    void HookChildPropertyChanged(const winrt::FrameworkElement& child);
    void UnhookChildPropertyChanged(bool isInDestructor);

    // Event handlers
    void OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnChildPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

private:
    std::shared_ptr<ScrollInputHelper> m_scrollInputHelper{ nullptr };
    winrt::Visual m_targetVisual{ nullptr };
    winrt::CompositionPropertySet m_animatedVariables{ nullptr };
    winrt::ExpressionAnimation m_horizontalSourceStartOffsetExpression{ nullptr };
    winrt::ExpressionAnimation m_horizontalSourceEndOffsetExpression{ nullptr };
    winrt::ExpressionAnimation m_verticalSourceStartOffsetExpression{ nullptr };
    winrt::ExpressionAnimation m_verticalSourceEndOffsetExpression{ nullptr };
    winrt::ExpressionAnimation m_horizontalParallaxExpressionInternal{ nullptr };
    winrt::ExpressionAnimation m_verticalParallaxExpressionInternal{ nullptr };
    bool m_isHorizontalAnimationStarted{ false };
    bool m_isVerticalAnimationStarted{ false };

    // Event Tokens
    winrt::event_token m_loadedToken{ 0 };
    winrt::event_token m_sizeChangedToken{ 0 };
    winrt::event_token m_childHorizontalAlignmentChangedToken{ 0 };
    winrt::event_token m_childVerticalAlignmentChangedToken{ 0 };
    tracker_ref<winrt::FrameworkElement> m_currentListeningChild{ this };

    // Property names being targeted for the ParallaxView.Child's Visual.
    // RedStone v1 case:
    static constexpr wstring_view s_transformMatrixTranslateXPropertyName{ L"TransformMatrix._41"sv };
    static constexpr wstring_view s_transformMatrixTranslateYPropertyName{ L"TransformMatrix._42"sv };
    // RedStone v2 and higher case:
    static constexpr wstring_view s_translationPropertyName{ L"Translation"sv };
    static constexpr wstring_view s_translationXPropertyName{ L"Translation.X"sv };
    static constexpr wstring_view s_translationYPropertyName{ L"Translation.Y"sv };
};
