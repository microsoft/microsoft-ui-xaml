// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <RatingItemInfo.h>

#include "RatingControl.g.h"
#include "RatingControl.properties.h"
#include "DispatcherHelper.h"

enum class RatingControlStates
{
    Disabled = 0,
    Set = 1,
    PointerOverSet = 2,
    PointerOverPlaceholder = 3, // Also functions as the pointer over unset state at the moment
    Placeholder = 4,
    Unset = 5,
    Null = 6
};

enum class RatingInfoType
{
    None,
    Font,
    Image
};

class RatingControl : 
    public ReferenceTracker<RatingControl, winrt::implementation::RatingControlT>,
    public RatingControlProperties
{
public:
    RatingControl();
    ~RatingControl();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    // Property changed handler.
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

public:
    // IUIElement / IUIElementOverridesHelper
    winrt::AutomationPeer OnCreateAutomationPeer();

private:
    // Methods that control rendering
    void StampOutRatingItems();
    void ReRenderCaption();
    void UpdateRatingItemsAppearance();
    void ResetControlWidth();

    // Methods that handle data
    void ChangeRatingBy(double increase, bool originatedFromMouse);
    void SetRatingTo(double newRating, bool originatedFromMouse);

    // Custom DependencyProperty changed event handlers
    void OnCaptionChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnFontFamilyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnInitialSetValueChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsClearEnabledChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsReadOnlyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnItemInfoChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMaxRatingChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnPlaceholderValueChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnValueChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnIsEnabledChanged(const winrt::IInspectable& sender, const winrt::DependencyPropertyChangedEventArgs& args);

    // Internal event handlers:
    void OnCaptionSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnPointerCancelledBackgroundStackPanel(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerCaptureLostBackgroundStackPanel(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerMovedOverBackgroundStackPanel(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerEnteredBackgroundStackPanel(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerExitedBackgroundStackPanel(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void PointerExitedImpl(const winrt::PointerRoutedEventArgs& args, bool resetScaleAnimation = true);
    void OnPointerPressedBackgroundStackPanel(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args); 
    void OnPointerReleasedBackgroundStackPanel(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnTextScaleFactorChanged(const winrt::UISettings& setting, const winrt::IInspectable& args);

    // Layout calculation helpers, and animations
    double CalculateTotalRatingControlWidth();
    double CalculateStarCenter(int starIndex);
    double CalculateActualRatingWidth();
    void ApplyScaleExpressionAnimation(const winrt::UIElement& uiElement, int starIndex);
    void PopulateStackPanelWithItems(wstring_view templateName, const winrt::StackPanel& stackPanel, RatingControlStates state);
    void CustomizeRatingItem(const winrt::UIElement& ui, RatingControlStates type);
    void CustomizeStackPanel(const winrt::StackPanel& stackPanel, RatingControlStates state);
    inline bool IsItemInfoPresentAndFontInfo()
    {
        return m_infoType == RatingInfoType::Font;
    };
    inline bool IsItemInfoPresentAndImageInfo()
    {
        return m_infoType == RatingInfoType::Image;
    };

    winrt::hstring GetAppropriateGlyph(RatingControlStates type);
    winrt::hstring GetNextGlyphIfNull(winrt::hstring glyph, RatingControlStates fallbackType = RatingControlStates::Set);
    winrt::ImageSource GetAppropriateImageSource(RatingControlStates type);
    winrt::ImageSource GetNextImageIfNull(winrt::ImageSource image, RatingControlStates fallbackType = RatingControlStates::Set);

public:
    // IControlOverrides / IControlOverridesHelper
    void OnKeyDown(winrt::KeyRoutedEventArgs const& e);

    // IControlOverrides6 / IControlOverrides6Helper
    void OnPreviewKeyDown(winrt::KeyRoutedEventArgs const& e);
    void OnPreviewKeyUp(winrt::KeyRoutedEventArgs const& e);

private:

    void OnFocusEngaged(const winrt::Control& sender, const winrt::FocusEngagedEventArgs& args);
    void OnFocusDisengaged(const winrt::Control& sender, const winrt::FocusDisengagedEventArgs& args);

    void EnterGamepadEngagementMode();
    void ExitGamepadEngagementMode();

    void RecycleEvents(bool useSafeGet = false);

    double CoerceValueBetweenMinAndMax(double value);

    float RenderingRatingFontSize();
    float ActualRatingFontSize();
    double ItemSpacing();
    void UpdateCaptionMargins();

    // Private members
    tracker_ref<winrt::TextBlock> m_captionTextBlock{ this };

    winrt::CompositionPropertySet m_sharedPointerPropertySet{ nullptr };

    tracker_ref<winrt::StackPanel> m_backgroundStackPanel{ this };
    tracker_ref<winrt::StackPanel> m_foregroundStackPanel{ this }; 

    bool m_isPointerOver{ false };
    bool m_isPointerDown{ false };
    double m_mousePercentage{ 0.0 };

    RatingInfoType m_infoType{ RatingInfoType::Font };

    // Holds the value of the Rating control at the moment of engagement,
    // used to handle cancel-disengagements where we reset the value.
    double m_preEngagementValue{ 0.0 };
    bool m_disengagedWithA{ false };
    bool m_shouldDiscardValue{ true };

    winrt::event_token m_pointerCancelledToken{};
    winrt::event_token m_pointerCaptureLostToken{};
    winrt::event_token m_pointerMovedToken{};
    winrt::event_token m_pointerEnteredToken{};
    winrt::event_token m_pointerExitedToken{};
    winrt::event_token m_pointerPressedToken{};
    winrt::event_token m_pointerReleasedToken{};
    winrt::event_token m_captionSizeChangedToken{};
    winrt::event_token m_fontFamilyChangedToken{};

    winrt::UISettings::TextScaleFactorChanged_revoker m_textScaleChangedRevoker{};
    static winrt::UISettings GetUISettings();

    DispatcherHelper m_dispatcherHelper{ *this };
};
