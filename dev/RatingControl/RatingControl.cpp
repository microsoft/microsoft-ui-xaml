// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RatingControl.h"
#include "RatingControlAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "XamlControlsResources.h"

#include <RatingItemFontInfo.h>
#include <RatingItemImageInfo.h>

const float c_horizontalScaleAnimationCenterPoint = 0.5f;
const float c_verticalScaleAnimationCenterPoint = 0.8f;
const winrt::Thickness c_focusVisualMargin = { -8, -7, -8, 0 };
const int c_defaultRatingFontSizeForRendering = 32; // (32 = 2 * [default fontsize] -- because of double size rendering), remove when MSFT #10030063 is done
const int c_defaultItemSpacing = 8;

const float c_mouseOverScale = 0.8f;
const float c_touchOverScale = 1.0f;
const float c_noPointerOverMagicNumber = -100;

// 22 = 20(compensate for the -20 margin on StackPanel) + 2(magic number makes the text and star center-aligned)
const float c_defaultCaptionTopMargin = 22;

const int c_noValueSetSentinel = -1;

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
    // MSFT #10030063 Replacing with Rating size DPs
    return (float) (c_defaultRatingFontSizeForRendering * GetUISettings().TextScaleFactor());
}

float RatingControl::ActualRatingFontSize()
{
    return RenderingRatingFontSize() / 2;
}

// TODO MSFT #10030063: Convert to itemspacing DP
double RatingControl::ItemSpacing()
{
    // Stars are rendered 2x size and we use expression animation to shrink them down to desired size,
    // which will create those spacings (not system margin).
    // Since text scale factor won't affect system margins,
    // when stars get bigger, the spacing will become smaller.
    // Therefore we should include TextScaleFactor when calculating item spacing
    // in order to get correct total width and star center positions.
    const double defaultFontSize = c_defaultRatingFontSizeForRendering / 2;
    return c_defaultItemSpacing - (GetUISettings().TextScaleFactor() - 1.0) * defaultFontSize / 2;
}

void RatingControl::UpdateCaptionMargins()
{
    // We manually set margins to caption text to make it center-aligned with the stars
    // because star vertical center is 0.8 instead of the normal 0.5.
    // When text scale changes we need to update top margin to make the text follow start center.
    if (auto captionTextBlock = m_captionTextBlock.safe_get())
    {
        double textScaleFactor = GetUISettings().TextScaleFactor();
        winrt::Thickness margin = captionTextBlock.Margin();
        margin.Top = c_defaultCaptionTopMargin - (ActualRatingFontSize() * c_verticalScaleAnimationCenterPoint);

        captionTextBlock.Margin(margin);
    }
}

void RatingControl::OnApplyTemplate()
{
    RecycleEvents();

    // Retrieve pointers to stable controls 
    winrt::IControlProtected thisAsControlProtected = *this;

    if (auto captionTextBlock = GetTemplateChildT<winrt::TextBlock>(L"Caption", thisAsControlProtected))
    {
        m_captionTextBlock.set(captionTextBlock);
        m_captionSizeChangedToken = captionTextBlock.SizeChanged({ this, &RatingControl::OnCaptionSizeChanged });
        UpdateCaptionMargins();
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

    if (SharedHelpers::IsRS1OrHigher())
    {
        // FUTURE: Ideally these would be in template overrides:

        // IsFocusEngagementEnabled means the control has to be "engaged" with 
        // using the A button before it actually receives key input from gamepad.
        FocusEngaged({ this, &RatingControl::OnFocusEngaged });
        FocusDisengaged({ this, &RatingControl::OnFocusDisengaged });
        IsFocusEngagementEnabled(true);

        // I've picked values so that these LOOK like the redlines, but these
        // values are not actually from the redlines because the redlines don't
        // consistently pick "distance from glyph"/"distance from edge of textbox"
        // so it's not possible to actually just have a consistent sizing model
        // here based on the redlines.
        FocusVisualMargin(c_focusVisualMargin);
    }

    IsEnabledChanged({ this, &RatingControl::OnIsEnabledChanged });
    m_fontFamilyChangedToken.value = RegisterPropertyChangedCallback(
        winrt::Control::FontFamilyProperty(), { this, &RatingControl::OnFontFamilyChanged });

    winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    winrt::Compositor comp = visual.Compositor();

    m_sharedPointerPropertySet = comp.CreatePropertySet();

    m_sharedPointerPropertySet.InsertScalar(L"starsScaleFocalPoint", c_noPointerOverMagicNumber);
    m_sharedPointerPropertySet.InsertScalar(L"pointerScalar", c_mouseOverScale);
 
    StampOutRatingItems();
    m_textScaleChangedRevoker = GetUISettings().TextScaleFactorChanged(winrt::auto_revoke, { this, &RatingControl::OnTextScaleFactorChanged });
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
    
    UpdateRatingItemsAppearance();
}

void RatingControl::ReRenderCaption()
{
    if (auto captionTextBlock = m_captionTextBlock.get())
    {
        ResetControlWidth();
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
            if (i + 1 > value)
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

        ResetControlWidth();
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

    // Star size = 16. 0.5 and 0.8 are just arbitrary center point chosen in design spec
    // 32 = star size * 2 because of the rendering at double size we do
    uiElementVisual.CenterPoint(winrt::float3(c_defaultRatingFontSizeForRendering * c_horizontalScaleAnimationCenterPoint, c_defaultRatingFontSizeForRendering * c_verticalScaleAnimationCenterPoint, 0.0f));
}

void RatingControl::PopulateStackPanelWithItems(wstring_view templateName, const winrt::StackPanel& stackPanel, RatingControlStates state)
{
    winrt::IInspectable lookup = winrt::Application::Current().Resources().Lookup(box_value(templateName));
    auto dt = lookup.as<winrt::DataTemplate>();

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
        }
    }
    else if (IsItemInfoPresentAndImageInfo())
    {
        if (auto image = ui.as<winrt::Image>())
        {
            image.Source(GetAppropriateImageSource(type));
            image.Width(RenderingRatingFontSize()); // 
            image.Height(RenderingRatingFontSize()); // MSFT #10030063 Replacing with Rating size DPs
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

void RatingControl::ResetControlWidth()
{
    const double newWidth = CalculateTotalRatingControlWidth();
    const winrt::Control thisAsControl = *this;
    thisAsControl.Width(newWidth);
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

        if (SharedHelpers::IsRS1OrHigher() && IsFocusEngaged() && ShouldEnableAnimation())
        {
            const double focalPoint = CalculateStarCenter((int)(ratingValue - 1.0));
            m_sharedPointerPropertySet.InsertScalar(L"starsScaleFocalPoint", static_cast<float>(focalPoint));
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
    ResetControlWidth();
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
}

void RatingControl::OnPointerMovedOverBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    if (!IsReadOnly())
    {
        const auto point = args.GetCurrentPoint(m_backgroundStackPanel.get());
        const float xPosition = point.Position().X;
        if (ShouldEnableAnimation())
        {
            m_sharedPointerPropertySet.InsertScalar(L"starsScaleFocalPoint", xPosition);
            auto deviceType = args.Pointer().PointerDeviceType();

            switch (deviceType)
            {
            case winrt::PointerDeviceType::Touch:
                m_sharedPointerPropertySet.InsertScalar(L"pointerScalar", c_touchOverScale);
                break;
            default: // mouse, TODO: distinguish pen later
                m_sharedPointerPropertySet.InsertScalar(L"pointerScalar", c_mouseOverScale);
                break;
            }
        }

        m_mousePercentage = static_cast<double>(xPosition) / CalculateActualRatingWidth();

        UpdateRatingItemsAppearance();
        args.Handled(true);
    }
}

void RatingControl::OnPointerEnteredBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    if (!IsReadOnly())
    {
        m_isPointerOver = true;
        args.Handled(true);
    }
}

void RatingControl::OnPointerExitedBackgroundStackPanel(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& args)
{
    PointerExitedImpl(args);
}

void RatingControl::PointerExitedImpl(const winrt::PointerRoutedEventArgs& args, bool resetScaleAnimation)
{
    auto point = args.GetCurrentPoint(m_backgroundStackPanel.get());

    if (resetScaleAnimation)
    {
        m_isPointerOver = false;
    }

    if (!m_isPointerDown)
    {
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

        if (SharedHelpers::IsRS1OrHigher())
        {
            winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::Invoke);
        }
    }

    if (m_isPointerDown)
    {
        m_isPointerDown = false;
        UpdateRatingItemsAppearance();
    }
}

double RatingControl::CalculateTotalRatingControlWidth()
{
    const double ratingStarsWidth = CalculateActualRatingWidth();
    const auto captionAsWinRT = unbox_value<winrt::hstring>(GetValue(s_CaptionProperty));
    double textSpacing = 0.0;

    if (captionAsWinRT.size() > 0)
    {
        textSpacing = ItemSpacing();
    }

    double captionWidth = 0.0;
    
    if (m_captionTextBlock)
    {
        captionWidth = m_captionTextBlock.get().ActualWidth();
    }

    return ratingStarsWidth + textSpacing + captionWidth;
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
    return (MaxRating() * ActualRatingFontSize()) + ((MaxRating() - 1) * ItemSpacing());
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
            if (SharedHelpers::IsRS1OrHigher())
            {
                winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::Focus);
            }
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
        return ;
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

bool RatingControl::ShouldEnableAnimation()
{
    // In ControlsResourceVersion2, animation is disabled.
    return !XamlControlsResources::IsUsingControlsResourcesVersion2() && SharedHelpers::IsAnimationsEnabled();
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

    if (SharedHelpers::IsRS1OrHigher())
    {
        winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::Invoke);
    }
    
    if (ShouldEnableAnimation())
    {
        const double focalPoint = CalculateStarCenter((int)(currentValue - 1.0));
        m_sharedPointerPropertySet.InsertScalar(L"starsScaleFocalPoint", static_cast<float>(focalPoint));
    }
}

void RatingControl::ExitGamepadEngagementMode()
{
    if (SharedHelpers::IsRS1OrHigher())
    {
        winrt::ElementSoundPlayer::Play(winrt::ElementSoundKind::GoBack);
    }

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

void RatingControl::OnTextScaleFactorChanged(const winrt::UISettings& setting, const winrt::IInspectable& args)
{
    // OnTextScaleFactorChanged happens in non-UI thread, use dispatcher to call StampOutRatingItems in UI thread.
    auto strongThis = get_strong();
    m_dispatcherHelper.RunAsync([strongThis]()
    {
        strongThis->StampOutRatingItems();
        strongThis->UpdateCaptionMargins();
    });
    
}

winrt::UISettings RatingControl::GetUISettings()
{
    static winrt::UISettings uiSettings = winrt::UISettings();
    return uiSettings;
}
