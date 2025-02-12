﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RatingControl.h"
#include "RatingControlAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "XamlControlsResources.h"

#include <RatingItemFontInfo.h>
#include <RatingItemImageInfo.h>

const float c_scaleAnimationCenterPointXValue = 16.0f;
const float c_scaleAnimationCenterPointYValue = 16.0f;

const int c_captionSpacing = 12;

const float c_mouseOverScale = 0.8f;
const float c_touchOverScale = 1.0f;
const float c_noPointerOverMagicNumber = -100;

const int c_noValueSetSentinel = -1;

const wchar_t c_fontSizeForRenderingKey[] = L"RatingControlFontSizeForRendering";
const wchar_t c_itemSpacingKey[] = L"RatingControlItemSpacing";
const wchar_t c_captionTopMarginKey[] = L"RatingControlCaptionTopMargin";


RatingControl::RatingControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RatingControl);

    SetDefaultStyleKey(this);
}

RatingControl::~RatingControl()
{
    // We only need to use safe_get in the deconstruction loop
    RecycleEvents(true /* useSafeGet */);
}

float RatingControl::RenderingRatingFontSize()
{
    MUX_ASSERT_MSG(m_scaledFontSizeForRendering >= 0, "RenderingRatingFontSize() should not be called prior to initializing m_scaledFontSizeForRendering.");

    // MSFT #10030063 Replacing with Rating size DPs
    return static_cast<float>(m_scaledFontSizeForRendering);
}

float RatingControl::ActualRatingFontSize()
{
    return RenderingRatingFontSize() / 2;
}

// TODO MSFT #10030063: Convert to itemspacing DP
double RatingControl::ItemSpacing()
{
    EnsureResourcesLoaded();

    return m_itemSpacing;
}

void RatingControl::OnApplyTemplate()
{
    RecycleEvents();

    // Retrieve pointers to stable controls 
    winrt::IControlProtected thisAsControlProtected = *this;

    if (auto captionStackPanel = GetTemplateChildT<winrt::StackPanel>(L"CaptionStackPanel", thisAsControlProtected))
    {
        m_captionStackPanel.set(captionStackPanel);
    }

    if (auto captionTextBlock = GetTemplateChildT<winrt::TextBlock>(L"Caption", thisAsControlProtected))
    {
        m_captionTextBlock.set(captionTextBlock);
        m_captionSizeChangedToken = captionTextBlock.SizeChanged({ this, &RatingControl::OnCaptionSizeChanged });
    }

    if (auto backgroundStackPanel = GetTemplateChildT<winrt::StackPanel>(L"RatingBackgroundStackPanel", thisAsControlProtected))
    {
        m_backgroundStackPanel.set(backgroundStackPanel);
        m_pointerCancelledToken = backgroundStackPanel.PointerCanceled({ this, &RatingControl::OnPointerCancelledBackgroundStackPanel });
        m_pointerCaptureLostToken = backgroundStackPanel.PointerCaptureLost({ this, &RatingControl::OnPointerCaptureLostBackgroundStackPanel });
        m_pointerMovedToken = backgroundStackPanel.PointerMoved({ this, &RatingControl::OnPointerMovedOverBackgroundStackPanel });
        m_pointerEnteredToken = backgroundStackPanel.PointerEntered({ this, &RatingControl::OnPointerEnteredBackgroundStackPanel });
        m_pointerExitedToken = backgroundStackPanel.PointerExited({ this, &RatingControl::OnPointerExitedBackgroundStackPanel });
        m_pointerPressedToken = backgroundStackPanel.PointerPressed({ this, &RatingControl::OnPointerPressedBackgroundStackPanel });
        m_pointerReleasedToken = backgroundStackPanel.PointerReleased({ this, &RatingControl::OnPointerReleasedBackgroundStackPanel });
    }

    m_foregroundStackPanel.set(GetTemplateChildT<winrt::StackPanel>(L"RatingForegroundStackPanel", thisAsControlProtected));

    m_backgroundStackPanelTranslateTransform.set(GetTemplateChildT<winrt::TranslateTransform>(L"RatingBackgroundStackPanelTranslateTransform", thisAsControlProtected));
    m_foregroundStackPanelTranslateTransform.set(GetTemplateChildT<winrt::TranslateTransform>(L"RatingForegroundStackPanelTranslateTransform", thisAsControlProtected));

    // FUTURE: Ideally these would be in template overrides:

    // IsFocusEngagementEnabled means the control has to be "engaged" with 
    // using the A button before it actually receives key input from gamepad.
    FocusEngaged({ this, &RatingControl::OnFocusEngaged });
    FocusDisengaged({ this, &RatingControl::OnFocusDisengaged });

    IsEnabledChanged({ this, &RatingControl::OnIsEnabledChanged });
    m_fontFamilyChangedToken.value = RegisterPropertyChangedCallback(
        winrt::Control::FontFamilyProperty(), { this, &RatingControl::OnFontFamilyChanged });

    winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    winrt::Compositor comp = visual.Compositor();

    m_sharedPointerPropertySet = comp.CreatePropertySet();

    m_sharedPointerPropertySet.InsertScalar(L"starsScaleFocalPoint", c_noPointerOverMagicNumber);
    m_sharedPointerPropertySet.InsertScalar(L"pointerScalar", c_mouseOverScale);

    StampOutRatingItems();
}


double RatingControl::CoerceValueBetweenMinAndMax(double value)
{
    if (value < 0.0) // Force all negative values to the sentinel "unset" value.
    {
        value = c_noValueSetSentinel;
    }
    else if (value <= 1.0)
    {
        value = 1.0;
    }
    else if (value > MaxRating())
    {
        value = MaxRating();
    }

    return value;
}

// IUIElement / IUIElementOverridesHelper
winrt::AutomationPeer RatingControl::OnCreateAutomationPeer()
{
    return winrt::make<RatingControlAutomationPeer>(*this);
}

// private methods 

// TODO: call me when font size changes, and stuff like that, glyph, etc
void RatingControl::StampOutRatingItems()
{
    if (!m_backgroundStackPanel || !m_foregroundStackPanel)
    {
        // OnApplyTemplate() hasn't executed yet, this is being called 
        // from a property value changed handler for markup set values.

        return;
    }

    // Before anything else, we need to retrieve the scaled font size.
    // Note that this is NOT the font size multiplied by the font scale factor,
    // as large font sizes are scaled less than small font sizes.
    // There isn't an API to retrieve this, so in lieu of that, we'll create a TextBlock
    // with the desired properties, measure it at infinity, and see what its desired width is.

    if (IsItemInfoPresentAndFontInfo())
    {
        EnsureResourcesLoaded();

        auto textBlock = winrt::TextBlock();
        textBlock.FontFamily(FontFamily());
        textBlock.Text(GetAppropriateGlyph(RatingControlStates::Set));
        textBlock.FontSize(m_fontSizeForRendering);
        textBlock.Measure({ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() });
        m_scaledFontSizeForRendering = textBlock.DesiredSize().Width;
    }
    else if (IsItemInfoPresentAndImageInfo())
    {
        // If we're using images rather than glyphs, then there's no text scaling
        // that will be happening.
        m_scaledFontSizeForRendering = m_fontSizeForRendering;
    }

    // Background initialization:

    m_backgroundStackPanel.get().Children().Clear();

    if (IsItemInfoPresentAndFontInfo())
    {
        PopulateStackPanelWithItems(L"BackgroundGlyphDefaultTemplate", m_backgroundStackPanel.get(), RatingControlStates::Unset);
    }
    else if (IsItemInfoPresentAndImageInfo())
    {
        PopulateStackPanelWithItems(L"BackgroundImageDefaultTemplate", m_backgroundStackPanel.get(), RatingControlStates::Unset);
    }

    // Foreground initialization:
    m_foregroundStackPanel.get().Children().Clear();
    if (IsItemInfoPresentAndFontInfo())
    {
        PopulateStackPanelWithItems(L"ForegroundGlyphDefaultTemplate", m_foregroundStackPanel.get(), RatingControlStates::Set);
    }
    else if (IsItemInfoPresentAndImageInfo())
    {
        PopulateStackPanelWithItems(L"ForegroundImageDefaultTemplate", m_foregroundStackPanel.get(), RatingControlStates::Set);
    }

    // The scale transform and margin cause the stars to be positioned at the top of the RatingControl.
    // We want them in the middle, so to achieve that, we'll additionally apply a y-transform that will
    // put the center of the stars in the center of the RatingControl.
    const auto yTranslation = (ActualHeight() - ActualRatingFontSize()) / 2;

    if (auto backgroundStackPanelTranslateTransform = m_backgroundStackPanelTranslateTransform.get())
    {
        backgroundStackPanelTranslateTransform.Y(yTranslation);
    }

    if (auto foregroundStackPanelTranslateTransform = m_foregroundStackPanelTranslateTransform.get())
    {
        foregroundStackPanelTranslateTransform.Y(yTranslation);
    }

    // If we have at least one item, we'll use the first item of the foreground stack panel as a representative element to determine some values.
    if (MaxRating() >= 1)
    {
        const auto firstItem = m_foregroundStackPanel.get().Children().GetAt(0).as<winrt::UIElement>();
        firstItem.Measure({ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() });
        const auto defaultItemSpacing = firstItem.DesiredSize().Width - ActualRatingFontSize();
        const auto netItemSpacing = ItemSpacing() - defaultItemSpacing;

        // We want the caption to be a set distance away from the right-hand side of the last item,
        // so we'll give it a left margin that accounts for the built-in item spacing.
        if (auto captionTextBlock = m_captionTextBlock.get())
        {
            auto margin = captionTextBlock.Margin();
            margin.Left = c_captionSpacing - defaultItemSpacing;
            captionTextBlock.Margin(margin);
        }

        // If we have at least two items, we'll need to apply the item spacing.
        // We'll calculate the default item spacing using the first item, and then
        // subtract it from the desired item spacing to get the Spacing property
        // to apply to the stack panels.
        if (MaxRating() >= 2)
        {
            m_backgroundStackPanel.get().Spacing(netItemSpacing);
            m_foregroundStackPanel.get().Spacing(netItemSpacing);
        }
    }

    UpdateRatingItemsAppearance();
}

void RatingControl::ReRenderCaption()
{
    if (auto captionTextBlock = m_captionTextBlock.get())
    {
        ResetControlSize();
    }
}

void RatingControl::UpdateRatingItemsAppearance()
{
    if (m_foregroundStackPanel)
    {
        // TODO: MSFT 11521414 - complete disabled state functionality

        const double placeholderValue = PlaceholderValue();
        const double ratingValue = Value();
        double value = 0.0;

        if (m_isPointerOver)
        {
            value = ceil(m_mousePercentage * MaxRating());
            if (ratingValue == c_noValueSetSentinel)
            {
                if (placeholderValue == -1)
                {
                    winrt::VisualStateManager::GoToState(*this, L"PointerOverPlaceholder", false);
                    CustomizeStackPanel(m_foregroundStackPanel.get(), RatingControlStates::PointerOverPlaceholder);
                }
                else
                {
                    winrt::VisualStateManager::GoToState(*this, L"PointerOverUnselected", false);
                    // The API is locked, so we can't change this part to be consistent any more:
                    CustomizeStackPanel(m_foregroundStackPanel.get(), RatingControlStates::PointerOverPlaceholder);
                }
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"PointerOverSet", false);
                CustomizeStackPanel(m_foregroundStackPanel.get(), RatingControlStates::PointerOverSet);
            }
        }
        else if (ratingValue > c_noValueSetSentinel)
        {
            value = ratingValue;
            winrt::VisualStateManager::GoToState(*this, L"Set", false);
            CustomizeStackPanel(m_foregroundStackPanel.get(), RatingControlStates::Set);
        }
        else if (placeholderValue > c_noValueSetSentinel)
        {
            value = placeholderValue;
            winrt::VisualStateManager::GoToState(*this, L"Placeholder", false);
            CustomizeStackPanel(m_foregroundStackPanel.get(), RatingControlStates::Placeholder);
        } // there's no "unset" state because the foreground items are simply cropped out

        if (!IsEnabled())
        {
            // TODO: MSFT 11521414 - complete disabled state functionality [merge this code block with ifs above]
            winrt::VisualStateManager::GoToState(*this, L"Disabled", false);
            CustomizeStackPanel(m_foregroundStackPanel.get(), RatingControlStates::Disabled);
        }

        unsigned int i = 0;
        for (const auto& uiElement : m_foregroundStackPanel.get().Children())
        {
            // Handle clips on stars
            float width = RenderingRatingFontSize();
            if (static_cast<double>(i) + 1 > value)
            {
                if (i < value)
                {
                    // partial stars
                    width *= static_cast<float>(value - floor(value));
                }
                else
                {
                    // empty stars
                    width = 0.0;
                }
            }

            winrt::Rect rect;
            rect.X = 0;
            rect.Y = 0;
            rect.Height = RenderingRatingFontSize();
            rect.Width = width;

            winrt::RectangleGeometry rg;
            rg.Rect(rect);
            uiElement.as<winrt::UIElement>().Clip(rg);

            i++;
        }

        ResetControlSize();
    }
}

void RatingControl::ApplyScaleExpressionAnimation(const winrt::UIElement& uiElement, int starIndex)
{
    const winrt::Visual uiElementVisual = winrt::ElementCompositionPreview::GetElementVisual(uiElement);
    const winrt::Compositor comp = uiElementVisual.Compositor();

    // starsScaleFocalPoint is updated in OnPointerMovedOverBackgroundStackPanel.
    // This expression uses the horizontal delta between pointer position and star center to calculate the star scale.
    // Star gets larger when pointer is closer to its center, and gets smaller when pointer moves further away.
    const winrt::ExpressionAnimation ea = comp.CreateExpressionAnimation(
        L"max( (-0.0005 * sharedPropertySet.pointerScalar * ((starCenterX - sharedPropertySet.starsScaleFocalPoint)*(starCenterX - sharedPropertySet.starsScaleFocalPoint))) + 1.0*sharedPropertySet.pointerScalar, 0.5)"
    );
    const auto starCenter = static_cast<float>(CalculateStarCenter(starIndex));
    ea.SetScalarParameter(L"starCenterX", starCenter);
    ea.SetReferenceParameter(L"sharedPropertySet", m_sharedPointerPropertySet);

    uiElementVisual.StartAnimation(L"Scale.X", ea);
    uiElementVisual.StartAnimation(L"Scale.Y", ea);

    EnsureResourcesLoaded();

    uiElementVisual.CenterPoint(winrt::float3(c_scaleAnimationCenterPointXValue, c_scaleAnimationCenterPointYValue, 0.0f));
}

void RatingControl::PopulateStackPanelWithItems(wstring_view templateName, const winrt::StackPanel& stackPanel, RatingControlStates state)
{
    winrt::IInspectable lookup = winrt::Application::Current().Resources().Lookup(box_value(templateName));
    auto dt = lookup.as<winrt::DataTemplate>();

    EnsureResourcesLoaded();

    for (int i = 0; i < MaxRating(); i++)
    {
        if (auto ui = dt.LoadContent().as<winrt::UIElement>())
        {
            CustomizeRatingItem(ui, state);
            stackPanel.Children().Append(ui);
            ApplyScaleExpressionAnimation(ui, i);
        }
    }
}

void RatingControl::CustomizeRatingItem(const winrt::UIElement& ui, RatingControlStates type)
{
    if (IsItemInfoPresentAndFontInfo())
    {
        if (auto textBlock = ui.as<winrt::TextBlock>())
        {
            textBlock.FontFamily(FontFamily());
            textBlock.Text(GetAppropriateGlyph(type));
            textBlock.FontSize(m_fontSizeForRendering);
        }
    }
    else if (IsItemInfoPresentAndImageInfo())
    {
        if (auto image = ui.as<winrt::Image>())
        {
            image.Source(GetAppropriateImageSource(type));
            image.Width(m_fontSizeForRendering); // 
            image.Height(m_fontSizeForRendering); // MSFT #10030063 Replacing with Rating size DPs
        }
    }
    else
    {
        MUX_FAIL_FAST_MSG("Runtime error, ItemInfo property is null");
    }
}

void RatingControl::CustomizeStackPanel(const winrt::StackPanel& stackPanel, RatingControlStates state)
{
    for (winrt::UIElement child : stackPanel.Children())
    {
        CustomizeRatingItem(child, state);
    }
}

winrt::hstring RatingControl::GetAppropriateGlyph(RatingControlStates type)
{
    if (!IsItemInfoPresentAndFontInfo())
    {
        MUX_FAIL_FAST_MSG("Runtime error, tried to retrieve a glyph when the ItemInfo is not a RatingItemGlyphInfo");
    }

    winrt::RatingItemFontInfo rifi = ItemInfo().as<winrt::RatingItemFontInfo>();

    switch (type)
    {
    case RatingControlStates::Disabled:
        return GetNextGlyphIfNull(rifi.DisabledGlyph(), RatingControlStates::Set);
        break;
    case RatingControlStates::PointerOverSet:
        return GetNextGlyphIfNull(rifi.PointerOverGlyph(), RatingControlStates::Set);
        break;
    case RatingControlStates::PointerOverPlaceholder:
        return GetNextGlyphIfNull(rifi.PointerOverPlaceholderGlyph(), RatingControlStates::Placeholder);
        break;
    case RatingControlStates::Placeholder:
        return GetNextGlyphIfNull(rifi.PlaceholderGlyph(), RatingControlStates::Set);
        break;
    case RatingControlStates::Unset:
        return GetNextGlyphIfNull(rifi.UnsetGlyph(), RatingControlStates::Set);
        break;
    case RatingControlStates::Null:
        return {};
        break;
    default:
        return rifi.Glyph(); // "Set" state
        break;
    }
}

winrt::hstring RatingControl::GetNextGlyphIfNull(winrt::hstring glyph, RatingControlStates fallbackType)
{
    if (glyph.size() == 0)
    {
        if (fallbackType == RatingControlStates::Null)
        {
            return {};
        }
        return GetAppropriateGlyph(fallbackType);
    }
    return glyph;
}

winrt::ImageSource RatingControl::GetAppropriateImageSource(RatingControlStates type)
{
    if (!IsItemInfoPresentAndImageInfo())
    {
        MUX_ASSERT_MSG(false, "Runtime error, tried to retrieve an image when the ItemInfo is not a RatingItemImageInfo");
    }

    winrt::RatingItemImageInfo imageInfo = ItemInfo().as<winrt::RatingItemImageInfo>();

    switch (type)
    {
    case RatingControlStates::Disabled:
        return GetNextImageIfNull(imageInfo.DisabledImage(), RatingControlStates::Set);
        break;
    case RatingControlStates::PointerOverSet:
        return GetNextImageIfNull(imageInfo.PointerOverImage(), RatingControlStates::Set);
        break;
    case RatingControlStates::PointerOverPlaceholder:
        return GetNextImageIfNull(imageInfo.PointerOverPlaceholderImage(), RatingControlStates::Placeholder);
        break;
    case RatingControlStates::Placeholder:
        return GetNextImageIfNull(imageInfo.PlaceholderImage(), RatingControlStates::Set);
        break;
    case RatingControlStates::Unset:
        return GetNextImageIfNull(imageInfo.UnsetImage(), RatingControlStates::Set);
        break;
    case RatingControlStates::Null:
        return nullptr;
        break;
    default:
        return imageInfo.Image(); // "Set" state
        break;
    }
}

winrt::ImageSource RatingControl::GetNextImageIfNull(winrt::ImageSource image, RatingControlStates fallbackType)
{
    if (!image)
    {
        if (fallbackType == RatingControlStates::Null)
        {
            return nullptr;
        }
        return GetAppropriateImageSource(fallbackType);
    }
    return image;
}

void RatingControl::ResetControlSize()
{
    Width(CalculateTotalRatingControlWidth());
    Height(m_fontSizeForRendering);
}

void RatingControl::ChangeRatingBy(double change, bool originatedFromMouse)
{
    if (change != 0.0)
    {
        double ratingValue = 0.0;
        double oldRatingValue = Value();
        if (oldRatingValue != c_noValueSetSentinel)
        {
            // If the Value was programmatically set to a fraction, drop that fraction before we modify it
            if ((int)Value() != Value())
            {
                if (change == -1)
                {
                    ratingValue = (int)Value();
                }
                else
                {
                    ratingValue = (int)Value() + change;
                }
            }
            else
            {
                oldRatingValue = ratingValue = oldRatingValue;
                ratingValue += change;
            }
        }
        else
        {
            ratingValue = InitialSetValue();
        }

        SetRatingTo(ratingValue, originatedFromMouse);
    }
}

void RatingControl::SetRatingTo(double newRating, bool originatedFromMouse)
{
    double ratingValue = 0.0;
    const double oldRatingValue = Value();

    ratingValue = std::min(newRating, static_cast<double>(MaxRating()));
    ratingValue = std::max(ratingValue, 0.0);

    // The base case, and the you have no rating, and you pressed left case [wherein nothing should happen]
    if (oldRatingValue > c_noValueSetSentinel || ratingValue != 0.0)
    {
        if (!IsClearEnabled() && ratingValue <= 0.0)
        {
            Value(1.0);
        }
        else if (ratingValue == oldRatingValue && IsClearEnabled() && (ratingValue != MaxRating() || originatedFromMouse))
        {
            // If you increase the Rating via the keyboard/gamepad when it's maxed, the value should stay stable.
            // But if you click a star that represents the current Rating value, it should clear the rating.

            Value(c_noValueSetSentinel);
        }
        else if (ratingValue > 0.0)
        {
            Value(ratingValue);
        }
        else
        {
            Value(c_noValueSetSentinel);
        }

        // Notify that the Value has changed
        m_valueChangedEventSource(*this, nullptr);
    }
}

void RatingControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto property = args.Property();
    // Do coercion first.
    if (property == s_MaxRatingProperty)
    {
        // Enforce minimum MaxRating
        auto value = winrt::unbox_value<int>(args.NewValue());
        auto coercedValue = std::max(1, value);

        if (Value() > coercedValue)
        {
            Value(coercedValue);
        }

        if (PlaceholderValue() > coercedValue)
        {
            PlaceholderValue(coercedValue);
        }

        if (coercedValue != value)
        {
            SetValue(property, winrt::box_value(coercedValue));
            return;
        }
    }
    else if (property == s_PlaceholderValueProperty || property == s_ValueProperty)
    {
        const auto value = winrt::unbox_value<double>(args.NewValue());
        const auto coercedValue = CoerceValueBetweenMinAndMax(value);
        if (value != coercedValue)
        {
            SetValue(property, winrt::box_value(coercedValue));
            // early return, we'll come back to handle the change to the corced value.
            return;
        }
    }

    // Property value changed handling.
    if (property == s_CaptionProperty)
    {
        OnCaptionChanged(args);
    }
    else if (property == s_InitialSetValueProperty)
    {
        OnInitialSetValueChanged(args);
    }
    else if (property == s_IsClearEnabledProperty)
    {
        OnIsClearEnabledChanged(args);
    }
    else if (property == s_IsReadOnlyProperty)
    {
        OnIsReadOnlyChanged(args);
    }
    else if (property == s_ItemInfoProperty)
    {
        OnItemInfoChanged(args);
    }
    else if (property == s_MaxRatingProperty)
    {
        OnMaxRatingChanged(args);
    }
    else if (property == s_PlaceholderValueProperty)
    {
        OnPlaceholderValueChanged(args);
    }
    else if (property == s_ValueProperty)
    {
        OnValueChanged(args);
    }
}

void RatingControl::OnCaptionChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    ReRenderCaption();
}

void RatingControl::OnFontFamilyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    if (m_backgroundStackPanel) // We don't want to do this for the initial property set
    {
        for (int i = 0; i < MaxRating(); i++)
        {
            // FUTURE: handle image rating items
            if (auto backgroundTB = m_backgroundStackPanel.get().Children().GetAt(i).try_as<winrt::TextBlock>())
            {
                CustomizeRatingItem(backgroundTB, RatingControlStates::Unset);
            }

            if (auto foregroundTB = m_foregroundStackPanel.get().Children().GetAt(i).try_as<winrt::TextBlock>())
            {
                CustomizeRatingItem(foregroundTB, RatingControlStates::Set);
            }
        }
    }

    UpdateRatingItemsAppearance();
}

void RatingControl::OnInitialSetValueChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{

}

void RatingControl::OnIsClearEnabledChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{

}

void RatingControl::OnIsReadOnlyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    // TODO: Colour changes - see spec
}

void RatingControl::OnItemInfoChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    bool changedType = false;

    if (!ItemInfo())
    {
        m_infoType = RatingInfoType::None;
    }
    else if (ItemInfo().try_as<winrt::RatingItemFontInfo>())
    {
        if (m_infoType != RatingInfoType::Font && m_backgroundStackPanel /* prevent calling StampOutRatingItems() twice at initialisation */)
        {
            m_infoType = RatingInfoType::Font;
            StampOutRatingItems();
            changedType = true;
        }
    }
    else
    {
        if (m_infoType != RatingInfoType::Image)
        {
            m_infoType = RatingInfoType::Image;
            StampOutRatingItems();
            changedType = true;
        }
    }

    // We don't want to do this for the initial property set
    // Or if we just stamped them out
    if (m_backgroundStackPanel && !changedType)
    {
        for (int i = 0; i < MaxRating(); i++)
        {
            CustomizeRatingItem(m_backgroundStackPanel.get().Children().GetAt(i), RatingControlStates::Unset);
            CustomizeRatingItem(m_foregroundStackPanel.get().Children().GetAt(i), RatingControlStates::Set);
        }
    }

    UpdateRatingItemsAppearance();
}

void RatingControl::OnMaxRatingChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    StampOutRatingItems();
}

void RatingControl::OnPlaceholderValueChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    UpdateRatingItemsAppearance();
}

void RatingControl::OnValueChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    // Fire property change for UIA
    if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        auto ratingPeer = peer.as<winrt::RatingControlAutomationPeer>();
        winrt::get_self<RatingControlAutomationPeer>(ratingPeer)->RaisePropertyChangedEvent(Value());
    }

    UpdateRatingItemsAppearance();
}

void RatingControl::OnIsEnabledChanged(const winrt::IInspectable& /*sender*/, const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    // MSFT 11521414 TODO: change states (add a state)
    UpdateRatingItemsAppearance();
}

void RatingControl::OnCaptionSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& /*args*/)
{
    // The caption's size changing means that the text scale factor has been updated and applied.
    // As such, we should re-run sizing and layout when this occurs.
    m_scaledFontSizeForRendering = -1;

    StampOutRatingItems();
    ResetControlSize();
}

void RatingControl::OnPointerCancelledBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    PointerExitedImpl(args);
}

void RatingControl::OnPointerCaptureLostBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    // We capture the pointer because we want to support the drag off the
    // left side to clear the rating scenario. However, this means that
    // when we simply click to set values - we get here, but we don't want
    // to reset the scaling on the stars underneath the pointer.
    PointerExitedImpl(args, false /* resetScaleAnimation */);
    m_hasPointerCapture = false;
}

void RatingControl::OnPointerMovedOverBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    if (!IsReadOnly())
    {
        if (auto backgroundStackPanel = m_backgroundStackPanel.get())
        {
            const auto point = args.GetCurrentPoint(backgroundStackPanel);
            const float xPosition = point.Position().X;

            m_mousePercentage = static_cast<double>(xPosition - m_firstItemOffset) / CalculateActualRatingWidth();

            UpdateRatingItemsAppearance();
            args.Handled(true);
        }
    }
}

void RatingControl::OnPointerEnteredBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    if (!IsReadOnly())
    {
        m_isPointerOver = true;

        if (auto backgroundStackPanel = m_backgroundStackPanel.get())
        {
            if (MaxRating() >= 1)
            {
                auto firstItem = backgroundStackPanel.Children().GetAt(0).as<winrt::UIElement>();
                auto firstItemOffsetPoint = firstItem.TransformToVisual(backgroundStackPanel).TransformPoint({ 0, 0 });
                m_firstItemOffset = firstItemOffsetPoint.X;
            }
        }

        args.Handled(true);
    }
}

void RatingControl::OnPointerExitedBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    PointerExitedImpl(args);
}

void RatingControl::PointerExitedImpl(const winrt::PointerRoutedEventArgs& args, bool resetScaleAnimation)
{
    if (resetScaleAnimation)
    {
        m_isPointerOver = false;
    }

    if (!m_isPointerDown)
    {
        // Only clear pointer capture when not holding pointer down for drag left to clear value scenario.
        if (m_hasPointerCapture)
        {
            m_backgroundStackPanel.get().ReleasePointerCapture(args.Pointer());
            m_hasPointerCapture = false;
        }

        if (resetScaleAnimation)
        {
            m_sharedPointerPropertySet.InsertScalar(L"starsScaleFocalPoint", c_noPointerOverMagicNumber);
        }
        UpdateRatingItemsAppearance();
    }

    args.Handled(true);
}

void RatingControl::OnPointerPressedBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    if (!IsReadOnly())
    {
        m_isPointerDown = true;

        // We capture the pointer on pointer down because we want to support
        // the drag off the left side to clear the rating scenario.
        m_backgroundStackPanel.get().CapturePointer(args.Pointer());
        m_hasPointerCapture = true;
    }
}

void RatingControl::OnPointerReleasedBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    if (!IsReadOnly())
    {
        const auto point = args.GetCurrentPoint(m_backgroundStackPanel.get());
        const auto xPosition = point.Position().X;

        const double mousePercentage = xPosition / CalculateActualRatingWidth();
        SetRatingTo(ceil(mousePercentage * MaxRating()), true);

        winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::Invoke);
    }

    if (m_isPointerDown)
    {
        m_isPointerDown = false;
        UpdateRatingItemsAppearance();
    }
}

double RatingControl::CalculateTotalRatingControlWidth()
{
    double totalWidth = CalculateActualRatingWidth();

    // If we have a non-empty caption, we also need to account for both its width and the spacing that comes before it.
    if (auto captionTextBlock = m_captionTextBlock.get())
    {
        const auto captionAsWinRT = unbox_value<winrt::hstring>(GetValue(s_CaptionProperty));

        if (captionAsWinRT.size() > 0)
        {
            totalWidth += c_captionSpacing + captionTextBlock.ActualWidth();
        }
    }

    return totalWidth;
}

double RatingControl::CalculateStarCenter(int starIndex)
{
    // TODO: sub in real API DP values
    // MSFT #10030063
    // [real Rating Size * (starIndex + 0.5)] + (starIndex * itemSpacing)
    return (ActualRatingFontSize() * (starIndex + 0.5)) + (starIndex * ItemSpacing());
}

double RatingControl::CalculateActualRatingWidth()
{
    // TODO: replace hardcoding
    // MSFT #10030063
    // (max rating * rating size) + ((max rating - 1) * item spacing)
    return (static_cast<double>(MaxRating()) * ActualRatingFontSize()) + ((static_cast<double>(MaxRating()) - 1) * ItemSpacing());
}

// IControlOverrides
void RatingControl::OnKeyDown(winrt::KeyRoutedEventArgs const& eventArgs)
{
    if (eventArgs.Handled())
    {
        return;
    }

    if (!IsReadOnly())
    {
        bool handled = false;
        winrt::VirtualKey key = eventArgs.as<winrt::KeyRoutedEventArgs>().Key();

        double flowDirectionReverser = 1.0;

        if (FlowDirection() == winrt::FlowDirection::RightToLeft)
        {
            flowDirectionReverser *= -1.0;
        }

        auto originalKey = eventArgs.as<winrt::KeyRoutedEventArgs>().OriginalKey();

        // Up down are right/left in keyboard only
        if (originalKey == winrt::VirtualKey::Up)
        {
            key = winrt::VirtualKey::Right;
            flowDirectionReverser = 1.0;
        }
        else if (originalKey == winrt::VirtualKey::Down)
        {
            key = winrt::VirtualKey::Left;
            flowDirectionReverser = 1.0;
        }

        if (originalKey == winrt::VirtualKey::GamepadDPadLeft || originalKey == winrt::VirtualKey::GamepadDPadRight ||
            originalKey == winrt::VirtualKey::GamepadLeftThumbstickLeft || originalKey == winrt::VirtualKey::GamepadLeftThumbstickRight)
        {
            winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::Focus);
        }

        switch (key)
        {
        case winrt::VirtualKey::Left:
            ChangeRatingBy(-1.0 * flowDirectionReverser, false);
            handled = true;
            break;
        case winrt::VirtualKey::Right:
            ChangeRatingBy(1.0 * flowDirectionReverser, false);
            handled = true;
            break;
        case winrt::VirtualKey::Home:
            SetRatingTo(0.0, false);
            handled = true;
            break;
        case winrt::VirtualKey::End:
            SetRatingTo(static_cast<double>(MaxRating()), false);
            handled = true;
            break;
        default:
            break;
        }

        eventArgs.Handled(handled);
    }

    __super::OnKeyDown(eventArgs);
}

// We use the same engagement model as Slider/sorta like ComboBox
// Pressing GamepadA engages, pressing either GamepadA or GamepadB disengages,
// where GamepadA commits the new value, and GamepadB discards and restores the old value.

// The reason we do this in the OnPreviewKey* virtuals is we need
// to beat the framework to handling this event. Because disengagement
// happens on key down, and engagement happens on key up...
// if we disengage on GamepadA, the framework would otherwise
// automatically reengage us.

// Order:
// OnPreviewKey* virtuals
// PreviewKey subscribed events
// [regular key events]

void RatingControl::OnPreviewKeyDown(winrt::KeyRoutedEventArgs const& eventArgs)
{
    if (eventArgs.Handled())
    {
        return;
    }

    if (!IsReadOnly() && IsFocusEngaged() && IsFocusEngagementEnabled())
    {
        auto originalKey = eventArgs.as<winrt::KeyRoutedEventArgs>().OriginalKey();
        if (originalKey == winrt::VirtualKey::GamepadA)
        {
            m_shouldDiscardValue = false;
            m_preEngagementValue = -1.0;
            RemoveFocusEngagement();
            m_disengagedWithA = true;
            eventArgs.Handled(true);
        }
        else if (originalKey == winrt::VirtualKey::GamepadB)
        {
            bool valueChanged = false;
            m_shouldDiscardValue = false;

            if (Value() != m_preEngagementValue)
            {
                valueChanged = true;
            }

            Value(m_preEngagementValue);

            if (valueChanged)
            {
                m_valueChangedEventSource(*this, nullptr);
            }

            m_preEngagementValue = -1.0;
            RemoveFocusEngagement();
            eventArgs.Handled(true);
        }
    }
}

void RatingControl::OnPreviewKeyUp(winrt::KeyRoutedEventArgs const& eventArgs)
{
    auto originalKey = eventArgs.as<winrt::KeyRoutedEventArgs>().OriginalKey();

    if (IsFocusEngagementEnabled() && originalKey == winrt::VirtualKey::GamepadA && m_disengagedWithA)
    {
        // Block the re-engagement
        m_disengagedWithA = false; // We want to do this regardless of handled
        eventArgs.Handled(true);
    }
}

void RatingControl::OnFocusEngaged(const winrt::Control& /*sender*/, const winrt::FocusEngagedEventArgs& /*args*/)
{
    if (!IsReadOnly())
    {
        EnterGamepadEngagementMode();
    }
}

void RatingControl::OnFocusDisengaged(const winrt::Control& /*sender*/, const winrt::FocusDisengagedEventArgs& /*args*/)
{
    // Revert value:
    // for catching programmatic disengagements, gamepad ones are handled in OnPreviewKeyDown
    if (m_shouldDiscardValue)
    {
        bool valueChanged = false;

        if (Value() != m_preEngagementValue)
        {
            valueChanged = true;
        }

        Value(m_preEngagementValue);
        m_preEngagementValue = -1.0f;

        if (valueChanged)
        {
            m_valueChangedEventSource(*this, nullptr);
        }
    }

    ExitGamepadEngagementMode();
}

void RatingControl::EnterGamepadEngagementMode()
{
    double currentValue = Value();
    m_shouldDiscardValue = true;

    if (currentValue == c_noValueSetSentinel)
    {
        Value(InitialSetValue());
        // Notify that the Value has changed
        m_valueChangedEventSource(*this, nullptr);
        currentValue = InitialSetValue();
        m_preEngagementValue = -1;
    }
    else
    {
        currentValue = Value();
        m_preEngagementValue = currentValue;
    }

    winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::Invoke);
}

void RatingControl::ExitGamepadEngagementMode()
{
    winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::GoBack);

    m_sharedPointerPropertySet.InsertScalar(L"starsScaleFocalPoint", c_noPointerOverMagicNumber);
    m_disengagedWithA = false;
}

void RatingControl::RecycleEvents(bool useSafeGet)
{
    if (auto backgroundStackPanel = m_backgroundStackPanel.safe_get(useSafeGet))
    {
        if (m_pointerCancelledToken.value)
        {
            backgroundStackPanel.PointerCanceled(m_pointerCancelledToken);
            m_pointerCancelledToken.value = 0;
        }

        if (m_pointerCaptureLostToken.value)
        {
            backgroundStackPanel.PointerCaptureLost(m_pointerCaptureLostToken);
            m_pointerCaptureLostToken.value = 0;
        }

        if (m_pointerMovedToken.value)
        {
            backgroundStackPanel.PointerMoved(m_pointerMovedToken);
            m_pointerMovedToken.value = 0;
        }

        if (m_pointerEnteredToken.value)
        {
            backgroundStackPanel.PointerEntered(m_pointerEnteredToken);
            m_pointerEnteredToken.value = 0;
        }

        if (m_pointerExitedToken.value)
        {
            backgroundStackPanel.PointerExited(m_pointerExitedToken);
            m_pointerExitedToken.value = 0;
        }

        if (m_pointerPressedToken.value)
        {
            backgroundStackPanel.PointerPressed(m_pointerPressedToken);
            m_pointerPressedToken.value = 0;
        }

        if (m_pointerReleasedToken.value)
        {
            backgroundStackPanel.PointerReleased(m_pointerReleasedToken);
            m_pointerReleasedToken.value = 0;
        }
    }

    if (auto captionTextBlock = m_captionTextBlock.safe_get(useSafeGet))
    {
        if (m_captionSizeChangedToken.value)
        {
            captionTextBlock.SizeChanged(m_captionSizeChangedToken);
            m_captionSizeChangedToken.value = 0;
        }
    }
}

void RatingControl::EnsureResourcesLoaded()
{
    if (!m_resourcesLoaded)
    {
        auto fontSizeForRenderingKey = box_value(c_fontSizeForRenderingKey);
        auto itemSpacingKey = box_value(c_itemSpacingKey);
        auto captionTopMarginKey = box_value(c_captionTopMarginKey);

        if (Resources().HasKey(fontSizeForRenderingKey))
        {
            m_fontSizeForRendering = unbox_value<double>(Resources().Lookup(fontSizeForRenderingKey));
        }
        else if (winrt::Application::Current().Resources().HasKey(fontSizeForRenderingKey))
        {
            m_fontSizeForRendering = unbox_value<double>(winrt::Application::Current().Resources().Lookup(fontSizeForRenderingKey));
        }
        else
        {
            m_fontSizeForRendering = c_defaultFontSizeForRendering;
        }

        if (Resources().HasKey(itemSpacingKey))
        {
            m_itemSpacing = unbox_value<double>(Resources().Lookup(itemSpacingKey));
        }
        else if (winrt::Application::Current().Resources().HasKey(itemSpacingKey))
        {
            m_itemSpacing = unbox_value<double>(winrt::Application::Current().Resources().Lookup(itemSpacingKey));
        }
        else
        {
            m_itemSpacing = c_defaultItemSpacing;
        }

        if (Resources().HasKey(captionTopMarginKey))
        {
            m_captionTopMargin = unbox_value<double>(Resources().Lookup(captionTopMarginKey));
        }
        else if (winrt::Application::Current().Resources().HasKey(captionTopMarginKey))
        {
            m_captionTopMargin = unbox_value<double>(winrt::Application::Current().Resources().Lookup(captionTopMarginKey));
        }
        else
        {
            m_captionTopMargin = c_defaultCaptionTopMargin;
        }

        m_resourcesLoaded = true;
    }
}
