// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Abstraction layer between the ParallaxView and its ScrollViewer/ScrollPresenter source.
class ScrollInputHelper
{
public:
    ScrollInputHelper(const ITrackerHandleManager* owner,
        std::function<void(bool, bool)> infoChangedFunction);
    ~ScrollInputHelper();

    winrt::UIElement TargetElement() const;
    winrt::CompositionPropertySet SourcePropertySet() const;
    bool IsTargetElementInSource() const;

    winrt::hstring GetSourceOffsetPropertyName(winrt::Orientation orientation) const;
    winrt::hstring GetSourceScalePropertyName() const;
    double GetOffsetFromScrollContentElement(const winrt::UIElement& element, winrt::Orientation orientation) const;
    double GetMaxUnderpanOffset(winrt::Orientation orientation) const;
    double GetMaxOverpanOffset(winrt::Orientation orientation) const;
    double GetContentSize(winrt::Orientation orientation) const;
    double GetViewportSize(winrt::Orientation orientation) const;
    void SetSourceElement(const winrt::UIElement& sourceElement);
    void SetTargetElement(const winrt::UIElement& targetElement);

private:
    static winrt::RichEditBox GetRichEditBoxParent(const winrt::DependencyObject& childElement);
    static void GetChildScrollPresenterOrScrollViewer(
        const winrt::DependencyObject& rootElement,
        _Out_ winrt::ScrollPresenter* scrollPresenter,
        _Out_ winrt::FxScrollViewer* scrollViewer);
    winrt::UIElement GetScrollContentElement() const;
    winrt::HorizontalAlignment GetEffectiveHorizontalAlignment() const;
    winrt::VerticalAlignment GetEffectiveVerticalAlignment() const;
    winrt::FxZoomMode GetEffectiveZoomMode() const;

    void SetScrollViewer(const winrt::FxScrollViewer& scrollViewer);
    void SetScrollPresenter(const winrt::ScrollPresenter& scrollPresenter);

    void UpdateOutOfBoundsPanSize();
    void UpdateContentSize();
    void UpdateViewportSize();
    void UpdateSource(bool allowSourceElementLoadedHookup);
    void UpdateIsTargetElementInSource();
    void UpdateManipulationZoomMode();
    void UpdateManipulationAlignments();
    void UpdateInternalExpressionAnimations(bool horizontalInfoChanged, bool verticalInfoChanged, bool zoomInfoChanged);

    winrt::HorizontalAlignment ComputeHorizontalContentAlignment() const;
    winrt::VerticalAlignment ComputeVerticalContentAlignment() const;
    winrt::FxZoomMode ComputeZoomMode() const;

    bool IsScrollContentPresenterIScrollInfoProvider() const;

    void EnsureInternalSourcePropertySetAndExpressionAnimations();
    void StartInternalExpressionAnimations(const winrt::CompositionPropertySet& source);
    void StopInternalExpressionAnimations();

    void ProcessSourceElementChange(bool allowSourceElementLoadedHookup);
    void ProcessSourceControlTemplateChange();
    void ProcessTargetElementChange();
    void ProcessContentSizeChange();
    void ProcessScrollViewerContentChange();
    void ProcessScrollPresenterContentChange();
    void ProcessScrollViewerZoomModeChange();

    void OnSourceElementChanged(bool allowSourceElementLoadedHookup);
    void OnTargetElementChanged();
    void OnSourceInfoChanged(bool horizontalInfoChanged, bool verticalInfoChanged, bool zoomInfoChanged);

    // Event handlers
    void OnSourceElementLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnSourceElementPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnTargetElementLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnSourceSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnSourceContentSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnScrollViewerPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnScrollPresenterPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnScrollViewerContentPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnScrollViewerDirectManipulationStarted(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnScrollViewerDirectManipulationCompleted(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnRichEditBoxTextChanged(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnCompositionTargetRendering(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    void HookSourceElementLoaded();
    void UnhookSourceElementLoaded();
    void HookSourceControlTemplateChanged();
    void UnhookSourceControlTemplateChanged();
    void HookTargetElementLoaded();
    void UnhookTargetElementLoaded();
    void HookScrollPresenterPropertyChanged();
    void UnhookScrollPresenterPropertyChanged();
    void HookScrollPresenterContentPropertyChanged();
    void UnhookScrollPresenterContentPropertyChanged();
    void HookScrollViewerPropertyChanged();
    void UnhookScrollViewerPropertyChanged();
    void HookScrollViewerContentPropertyChanged();
    void UnhookScrollViewerContentPropertyChanged();
    void HookScrollViewerDirectManipulationStarted();
    void UnhookScrollViewerDirectManipulationStarted();
    void HookScrollViewerDirectManipulationCompleted();
    void UnhookScrollViewerDirectManipulationCompleted();
    void HookRichEditBoxTextChanged();
    void UnhookRichEditBoxTextChanged();
    void HookCompositionTargetRendering();
    void UnhookCompositionTargetRendering();

private:
    const ITrackerHandleManager* m_owner;
    std::function<void(bool, bool)> m_infoChangedFunction;

    tracker_ref<winrt::UIElement> m_sourceElement{ m_owner };
    tracker_ref<winrt::UIElement> m_targetElement{ m_owner };
    tracker_ref<winrt::FxScrollViewer> m_scrollViewer{ m_owner };
    tracker_ref<winrt::ScrollPresenter> m_scrollPresenter{ m_owner };
    tracker_ref<winrt::FrameworkElement> m_sourceContent{ m_owner };
    tracker_ref<winrt::RichEditBox> m_richEditBox{ m_owner };
    winrt::CompositionPropertySet m_internalSourcePropertySet{ nullptr };
    winrt::CompositionPropertySet m_sourcePropertySet{ nullptr };
    winrt::CompositionPropertySet m_scrollViewerPropertySet{ nullptr };
    winrt::ExpressionAnimation m_internalTranslationXExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_internalTranslationYExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_internalScaleExpressionAnimation{ nullptr };
    winrt::FxZoomMode m_manipulationZoomMode{ winrt::FxZoomMode::Disabled };
    winrt::HorizontalAlignment m_manipulationHorizontalAlignment{ winrt::HorizontalAlignment::Stretch };
    winrt::VerticalAlignment m_manipulationVerticalAlignment{ winrt::VerticalAlignment::Stretch };
    winrt::Size m_viewportSize{ 0.0f, 0.0f };
    winrt::Size m_contentSize{ 0.0f, 0.0f };
    winrt::Size m_outOfBoundsPanSize{ 0.0f, 0.0f };
    bool m_isTargetElementInSource{ false };
    bool m_isScrollViewerInDirectManipulation{ false };

    // Event Tokens
    winrt::event_token m_targetElementLoadedToken{ 0 };
    winrt::event_token m_sourceElementLoadedToken{ 0 };
    winrt::event_token m_sourceControlTemplateChangedToken{ 0 };
    winrt::event_token m_sourceSizeChangedToken{ 0 };
    winrt::event_token m_sourceContentSizeChangedToken{ 0 };
    winrt::event_token m_scrollViewerContentHorizontalAlignmentChangedToken{ 0 };
    winrt::event_token m_scrollViewerContentVerticalAlignmentChangedToken{ 0 };
    winrt::event_token m_scrollViewerContentChangedToken{ 0 };
    winrt::event_token m_scrollPresenterContentChangedToken{ 0 };
    winrt::event_token m_scrollViewerHorizontalContentAlignmentChangedToken{ 0 };
    winrt::event_token m_scrollViewerVerticalContentAlignmentChangedToken{ 0 };
    winrt::event_token m_scrollViewerZoomModeChangedToken{ 0 };
    winrt::event_token m_scrollViewerDirectManipulationStartedToken{ 0 };
    winrt::event_token m_scrollViewerDirectManipulationCompletedToken{ 0 };
    winrt::event_token m_richEditBoxTextChangedToken{ 0 };
    winrt::event_token m_renderingToken{ 0 };

    // Property names inside the composition property set returned by get_SourcePropertySet.
    static PCWSTR s_horizontalOffsetPropertyName;
    static PCWSTR s_verticalOffsetPropertyName;
    static PCWSTR s_scalePropertyName;
};

