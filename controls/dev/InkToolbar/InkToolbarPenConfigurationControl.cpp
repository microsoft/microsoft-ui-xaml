// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarPenConfigurationControl_Partial.cpp.
// PORTABLE logic preserved 1:1: color picker (PenColorPalette ListViewBase), stroke-width Slider
// (PenStrokeWidthSlider/EraserStrokeWidthSlider), selection <-> pen SelectedBrushIndex, slider <-> pen
// SelectedStrokeWidth, focus + keyboard/dial navigation, ColorNames tooltips, item source from the pen
// Palette. UpdatePreview / stroke preview is reimplemented as a lightweight 2D XAML Path (drawn in the
// current pen color + width) because the UWP live wincomp preview (IInkPresenterHost +
// ElementCompositionPreview) is not on the lift surface. High-contrast palette + monitoring is wired via
// AccessibilitySettings (the flyout swaps to a contrast-filtered palette in HC mode). STUBBED + documented
// (lift-unavailable): UpdateCurrentFlyoutBackgroundBrush (highlighter wincomp background), Surface Dial
// (ExternalActorMode/Refocus). The pen L3 XAML template must be ported for the parts to resolve.

#include "pch.h"
#include "common.h"
#include "InkToolbarPenConfigurationControl.h"
#include "InkToolbar.h"
#include "InkToolbarPenButton.h"
#include "InkToolbarTrace.h"

#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Windows.UI.ViewManagement.h>

// Sample-stroke coordinates for the pen-flyout preview (ported from UWP c_previewStrokeCoordinates):
// 103 (x,y) points describing a gently curved stroke, drawn in the current pen color + width.
static constexpr float s_previewStrokeCoordinates[] = {
    24.0f,28.5f,            26.175271f,26.452467f,  28.468156f,24.57762f,   30.860594f, 22.862812f, 33.253032f, 21.148004f,
    35.745021f,19.593234f,  38.3185f,18.185855f,    40.891979f,16.778476f,  43.546946f,15.518487f,  46.26534f,14.39324f,
    48.983733f,13.267993f,  51.765552f,12.277489f,  54.592735f,11.409078f,  57.419917f,10.540668f,  60.292463f,9.794352f,
    63.192308f,9.1574822f,  66.092153f,8.5206123f,  69.019299f,7.9931887f,  71.955681f,7.5625631f,  74.892063f,7.1319376f,
    77.837682f,6.7981102f,  80.774476f,6.5484328f,  83.711269f,6.2987555f,  86.639236f,6.1332282f,  89.540314f,6.0392029f,
    92.12168f,5.9271775f,   94.691318f,5.9313799f,  97.248453f,6.040089f,   99.805587f,6.1487981f,  102.35022f,6.3620141f,
    104.88157f,6.668016f,   107.41293f,6.974018f,   109.931f,7.3728059f,    112.43502f,7.852659f,   114.93904f,8.332512f,
    117.42901f,8.8934302f,  119.90414f,9.5236926f,  122.37928f,10.153955f,  124.83959f,10.853562f,  127.28429f,11.610792f,
    129.72899f,12.368022f,  132.15809f,13.182876f,  134.57081f,14.043632f,  136.98352f,14.904388f,  139.37986f,15.811047f,
    141.75904f,16.751888f,  144.13822f,17.692728f,  146.50025f,18.667751f,  148.84434f,19.665234f,  150.92055f,20.558561f,
    153.02009f,21.393996f,  155.13997f,22.171709f,  157.25986f,22.949423f,  159.40009f,23.669414f,  161.55771f,24.331854f,
    163.71533f,24.994294f,  165.89033f,25.599182f,  168.07973f,26.146689f,  170.26914f,26.694195f,  172.47295f,27.184321f,
    174.6882f,27.617235f,   176.90346f,28.050149f,  179.13015f,28.425851f,  181.3653f,28.744513f,   183.60046f,29.063175f,
    185.84408f,29.324795f,  188.0932f,29.529545f,   190.34231f,29.734295f,  192.59692f,29.882174f,  194.85406f,29.973352f,
    197.11119f,30.06453f,   199.37085f,30.099008f,  201.63006f,30.076955f,  203.88927f,30.054901f,  206.14803f,29.976318f,
    208.40338f,29.841374f,  210.65872f,29.70643f,   212.91064f,29.515126f,  215.15617f,29.267631f,  219.64724f,28.772643f,
    224.11274f,28.052893f,  228.52891f,27.109745f,  230.73699f,26.638171f,  232.93274f,26.110747f,  235.11319f,25.527643f,
    237.29363f,24.94454f,   239.45877f,24.305756f,  241.60564f,23.611464f,  243.7525f,22.917171f,   245.88109f,22.167369f,
    247.98843f,21.362228f,  250.09577f,20.557087f,  252.18187f,19.696606f,  254.24374f,18.780957f,  257.44071f,17.278777f,
    260.5682f,15.598548f,   263.55223f,13.698859f,  265.04425f,12.749015f,  266.50041f,11.744305f,  267.91145f,10.679554f,
    269.32249f,9.6148034f,  270.68842f,8.4900108f,  272.0f,7.3f };
static constexpr double c_previewStrokeMargin = 10.0;

InkToolbarPenConfigurationControl::~InkToolbarPenConfigurationControl()
{
    try
    {
        if (auto colorPicker = m_colorPicker.get())
        {
            if (m_colorPickerPointerPressedEventHandler) { colorPicker.RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_colorPickerPointerPressedEventHandler); }
            if (m_colorPickerPointerReleasedEventHandler) { colorPicker.RemoveHandler(winrt::UIElement::PointerReleasedEvent(), m_colorPickerPointerReleasedEventHandler); }
            if (m_colorPickerKeyDownEventHandler) { colorPicker.RemoveHandler(winrt::UIElement::KeyDownEvent(), m_colorPickerKeyDownEventHandler); }
        }
        if (auto slider = m_strokeWidthSlider.get())
        {
            auto sliderUI = slider.as<winrt::UIElement>();
            if (m_widthSliderPointerPressedEventHandler) { sliderUI.RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_widthSliderPointerPressedEventHandler); }
            if (m_widthSliderPointerReleasedEventHandler) { sliderUI.RemoveHandler(winrt::UIElement::PointerReleasedEvent(), m_widthSliderPointerReleasedEventHandler); }
            if (m_widthSliderKeyDownEventHandler) { sliderUI.RemoveHandler(winrt::UIElement::KeyDownEvent(), m_widthSliderKeyDownEventHandler); }
        }
        if (m_accessibilitySettings && m_highContrastChangedToken)
        {
            m_accessibilitySettings.HighContrastChanged(m_highContrastChangedToken);
        }
        if (auto grid = m_strokePreviewGrid.get())
        {
            if (m_previewGridSizeChangedToken) { grid.SizeChanged(m_previewGridSizeChangedToken); }
        }
    }
    catch (...)
    {
    }
}

void InkToolbarPenConfigurationControl::SetBindingData(winrt::InkToolbar const& inkToolbar, winrt::InkToolbarToolButton const& button)
{
    m_inkToolbar = winrt::make_weak(inkToolbar);

    if (auto penButton = button.try_as<winrt::InkToolbarPenButton>())
    {
        PenButton(penButton);
    }

    // Regenerating the item source will change the selected brush index, which triggers UpdatePreview().
    RegenerateItemSource();
}

void InkToolbarPenConfigurationControl::OnApplyTemplate()
{
    auto penButton = PenButton();

    ConfigureStrokeWidthSlider(nullptr);
    ConfigureStrokeWidthPreview();

    if (penButton)
    {
        ConfigureColorPicker(nullptr);
    }
    else
    {
        // The eraser pen L3 doesn't need the color picker.
        RemoveColorPicker(nullptr);
    }

    ConfigureLocalizableElements(nullptr);
    ConfigureHighContrast();
}

void InkToolbarPenConfigurationControl::ConfigureColorPicker(winrt::Control const& me)
{
    UNREFERENCED_PARAMETER(me);

    auto colorPicker = GetTemplateChild(L"PenColorPalette").try_as<winrt::ListViewBase>();
    if (!colorPicker)
    {
        return;
    }

    m_colorPicker = winrt::make_weak(colorPicker);

    auto selector = colorPicker.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::Selector>();
    m_selectionChangedToken = selector.SelectionChanged(
        winrt::SelectionChangedEventHandler([this](winrt::IInspectable const& s, winrt::SelectionChangedEventArgs const& e) { OnSelectedItemChanged(s, e); }));

    m_containerContentChangingToken = colorPicker.ContainerContentChanging(
        winrt::TypedEventHandler<winrt::ListViewBase, winrt::ContainerContentChangingEventArgs>(
            [this](winrt::ListViewBase const& s, winrt::ContainerContentChangingEventArgs const& e) { OnContainerContentChanging(s, e); }));

    m_colorPickerGotFocusToken = colorPicker.GotFocus(
        winrt::RoutedEventHandler([this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnColorPickerGotFocus(s, e); }));

    m_colorPickerPointerPressedEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnColorPickerPointerPressed(s, e); }));
    colorPicker.AddHandler(winrt::UIElement::PointerPressedEvent(), m_colorPickerPointerPressedEventHandler, true);
    m_colorPickerPointerReleasedEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnColorPickerPointerReleased(s, e); }));
    colorPicker.AddHandler(winrt::UIElement::PointerReleasedEvent(), m_colorPickerPointerReleasedEventHandler, true);
    m_colorPickerKeyDownEventHandler = winrt::box_value(winrt::KeyEventHandler([this](winrt::IInspectable const& s, winrt::KeyRoutedEventArgs const& e) { OnColorPickerKeyDown(s, e); }));
    colorPicker.AddHandler(winrt::UIElement::KeyDownEvent(), m_colorPickerKeyDownEventHandler, true);

    RegenerateItemSource();
}

// The PenConfigurationControl has two sliders (pen and eraser). Configure the relevant one, remove the other.
void InkToolbarPenConfigurationControl::ConfigureStrokeWidthSlider(winrt::Control const& me)
{
    UNREFERENCED_PARAMETER(me);

    auto penButton = PenButton();
    auto sliderName = penButton ? L"PenStrokeWidthSlider" : L"EraserStrokeWidthSlider";

    auto slider = GetTemplateChild(sliderName).try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBase>();
    if (!slider)
    {
        return;
    }

    m_strokeWidthSlider = winrt::make_weak(slider);

    m_widthSliderValueChangedToken = slider.ValueChanged(
        winrt::RangeBaseValueChangedEventHandler([this](winrt::IInspectable const& s, winrt::RangeBaseValueChangedEventArgs const& e) { OnStrokeWidthSliderValueChanged(s, e); }));

    auto sliderUI = slider.as<winrt::UIElement>();
    m_widthSliderGotFocusToken = slider.as<winrt::FrameworkElement>().GotFocus(
        winrt::RoutedEventHandler([this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnStrokeWidthSliderGotFocus(s, e); }));

    m_widthSliderPointerPressedEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnStrokeWidthSliderPointerPressed(s, e); }));
    sliderUI.AddHandler(winrt::UIElement::PointerPressedEvent(), m_widthSliderPointerPressedEventHandler, true);
    m_widthSliderPointerReleasedEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnStrokeWidthSliderPointerReleased(s, e); }));
    sliderUI.AddHandler(winrt::UIElement::PointerReleasedEvent(), m_widthSliderPointerReleasedEventHandler, true);
    m_widthSliderKeyDownEventHandler = winrt::box_value(winrt::KeyEventHandler([this](winrt::IInspectable const& s, winrt::KeyRoutedEventArgs const& e) { OnStrokeWidthKeyDown(s, e); }));
    sliderUI.AddHandler(winrt::UIElement::KeyDownEvent(), m_widthSliderKeyDownEventHandler, true);

    // Remove the other slider from the root grid.
    auto sliderToRemoveName = (!penButton) ? L"PenStrokeWidthSlider" : L"EraserStrokeWidthSlider";
    auto sliderToRemove = GetTemplateChild(sliderToRemoveName).try_as<winrt::UIElement>();
    if (auto rootGrid = GetTemplateChild(L"RootElement").try_as<winrt::Panel>())
    {
        if (sliderToRemove)
        {
            uint32_t index = 0;
            if (rootGrid.Children().IndexOf(sliderToRemove, index))
            {
                rootGrid.Children().RemoveAt(index);
            }
        }
    }
}

void InkToolbarPenConfigurationControl::ConfigureLocalizableElements(winrt::Control const& me)
{
    UNREFERENCED_PARAMETER(me);
    // Titles/automation names come from string resources (a lift resource gap) - left to the template's
    // default text. Structure preserved; nothing to set without a resource provider.
}

void InkToolbarPenConfigurationControl::RemoveColorPicker(winrt::Control const& me)
{
    UNREFERENCED_PARAMETER(me);

    auto colorPickerTitle = GetTemplateChild(L"PenColorPaletteTitle").try_as<winrt::UIElement>();
    auto colorPicker = GetTemplateChild(L"PenColorPalette").try_as<winrt::UIElement>();

    if (auto rootGrid = GetTemplateChild(L"RootElement").try_as<winrt::Panel>())
    {
        auto children = rootGrid.Children();
        uint32_t index = 0;
        if (colorPickerTitle && children.IndexOf(colorPickerTitle, index)) { children.RemoveAt(index); }
        if (colorPicker && children.IndexOf(colorPicker, index)) { children.RemoveAt(index); }
    }
}

// Built-in collections are bindable in C++/WinRT, so we set the pen Palette (an IVector<Brush>) directly as
// the ItemsSource (UWP wrapped it in a BindableVector). High-contrast palette substitution is a lift gap.
void InkToolbarPenConfigurationControl::RegenerateItemSource()
{
    auto colorPicker = m_colorPicker.get();
    if (!colorPicker)
    {
        return;   // Template not applied yet.
    }

    auto button = PenButton();
    winrt::Windows::Foundation::Collections::IVector<winrt::Brush> palette{ nullptr };
    if (button)
    {
        if (IsHighContrast())
        {
            int hcAdjust = 0; // UseSystemColorsWhenNecessary
            if (auto toolbar = m_inkToolbar.get())
            {
                hcAdjust = winrt::get_self<InkToolbar>(toolbar)->GetHighContrastAdjustmentValue();
            }
            palette = winrt::get_self<InkToolbarPenButton>(button)->GetHighContrastPalette(hcAdjust);
        }
        if (!palette)
        {
            palette = button.Palette();
        }
    }

    if (!palette)
    {
        colorPicker.ItemsSource(nullptr);
        return;
    }

    // Setting the items source triggers SelectionChanged; suppress preview updates during regeneration.
    m_regeneratingItemSource = true;
    colorPicker.ItemsSource(palette);

    auto selector = colorPicker.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::Selector>();
    int selectedIndex = button.SelectedBrushIndex();
    uint32_t paletteEntryCount = palette.Size();

    if (selectedIndex == -1 || (selectedIndex >= 0 && static_cast<uint32_t>(selectedIndex) < paletteEntryCount))
    {
        m_regeneratingItemSource = true;
        selector.SelectedIndex(selectedIndex);
    }

    m_regeneratingItemSource = false;
}

void InkToolbarPenConfigurationControl::OnSelectedItemChanged(winrt::IInspectable const& sender, winrt::SelectionChangedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);

    if (auto colorPicker = m_colorPicker.get())
    {
        auto selector = colorPicker.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::Selector>();
        int selectedIndex = selector.SelectedIndex();

        if (auto button = PenButton())
        {
            if (selectedIndex >= 0)
            {
                button.SelectedBrushIndex(selectedIndex);
            }
        }
    }

    if (!m_regeneratingItemSource)
    {
        UpdatePreview();
    }
}

void InkToolbarPenConfigurationControl::OnStrokeWidthSliderValueChanged(winrt::IInspectable const& sender, winrt::RangeBaseValueChangedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
    UpdatePreview();
}

// Attach accessibility info (color name) to the palette items.
void InkToolbarPenConfigurationControl::OnContainerContentChanging(winrt::ListViewBase const& sender, winrt::ContainerContentChangingEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);

    winrt::hstring colorText;
    if (auto brush = args.Item().try_as<winrt::SolidColorBrush>())
    {
        bool isGenericFormat = false;
        colorText = m_colorNames.GetColorName(brush.Color(), isGenericFormat);
    }
    else
    {
        colorText = m_nonSolidColorString;
    }

    if (auto container = args.ItemContainer())
    {
        winrt::ToolTipService::SetToolTip(container, winrt::box_value(colorText));
        winrt::AutomationProperties::SetName(container, colorText);
        winrt::AutomationProperties::SetAutomationId(container, colorText);
    }
}

void InkToolbarPenConfigurationControl::OnColorPickerGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);

    bool setMode = m_setMode;
    m_setMode = false;

    if (setMode)
    {
        if (auto toolbar = m_inkToolbar.get())
        {
            winrt::get_self<InkToolbar>(toolbar)->OnPenL3ColorPickerGotFocus();
        }
    }
    // UWP Refocus(ExternalActorMode::InkColor) dropped - dial unavailable.
}

void InkToolbarPenConfigurationControl::OnStrokeWidthSliderGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);

    bool setMode = m_setMode;
    m_setMode = false;

    if (setMode)
    {
        if (auto toolbar = m_inkToolbar.get())
        {
            winrt::get_self<InkToolbar>(toolbar)->OnPenL3StrokeWidthGotFocus();
        }
    }
}

// A pointer press in the L3 means the user intentionally changed the mode; allow the toolbar to set it.
void InkToolbarPenConfigurationControl::OnL3PointerPressed()
{
    m_setMode = true;
}

// A pointer release commits the change and dismisses the L3.
void InkToolbarPenConfigurationControl::OnL3PointerReleased(bool isColor)
{
    bool setMode = m_setMode;
    m_setMode = false;

    if (auto toolbar = m_inkToolbar.get())
    {
        auto self = winrt::get_self<InkToolbar>(toolbar);
        if (setMode)
        {
            if (isColor) { self->OnPenL3ColorPickerGotFocus(); }
            else { self->OnPenL3StrokeWidthGotFocus(); }
        }
        self->DismissL3(*this);
    }
}

void InkToolbarPenConfigurationControl::OnColorPickerPointerPressed(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
    OnL3PointerPressed();
}

void InkToolbarPenConfigurationControl::OnColorPickerPointerReleased(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
    OnL3PointerReleased(true);
}

void InkToolbarPenConfigurationControl::OnColorPickerKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
}

void InkToolbarPenConfigurationControl::OnStrokeWidthSliderPointerPressed(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
    OnL3PointerPressed();
}

void InkToolbarPenConfigurationControl::OnStrokeWidthSliderPointerReleased(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
    OnL3PointerReleased(false);
}

void InkToolbarPenConfigurationControl::OnStrokeWidthKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
}

void InkToolbarPenConfigurationControl::SetFocusToSelectedColor(winrt::FocusState focusState)
{
    auto colorPicker = m_colorPicker.get();
    if (!colorPicker)
    {
        return;
    }

    if (colorPicker.Items().Size() > 0)
    {
        auto selector = colorPicker.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::Selector>();
        auto item = selector.SelectedItem();
        if (auto container = colorPicker.ContainerFromItem(item).try_as<winrt::Control>())
        {
            container.Focus(focusState);
        }
    }
}

void InkToolbarPenConfigurationControl::SetFocusToWidthSlider(winrt::FocusState focusState)
{
    if (auto slider = m_strokeWidthSlider.get())
    {
        slider.as<winrt::Control>().Focus(focusState);
    }
}

void InkToolbarPenConfigurationControl::AdvanceColorSelection(bool forward)
{
    auto colorPicker = m_colorPicker.get();
    if (!colorPicker)
    {
        return;
    }

    colorPicker.as<winrt::Control>().Focus(winrt::FocusState::Keyboard);

    uint32_t itemCount = colorPicker.Items().Size();
    if (itemCount > 0)
    {
        auto selector = colorPicker.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::Selector>();
        int selectedIndex = selector.SelectedIndex();

        if (forward)
        {
            selectedIndex = (selectedIndex + 1) % static_cast<int>(itemCount);
        }
        else
        {
            selectedIndex = (0 == selectedIndex) ? static_cast<int>(itemCount) - 1 : (selectedIndex - 1);
        }

        selector.SelectedIndex(selectedIndex);
    }
}

bool InkToolbarPenConfigurationControl::AreSame(double a, double b)
{
    return std::fabs(a - b) < 0.1;
}

bool InkToolbarPenConfigurationControl::AdvanceStrokeWidthSelection(bool forward)
{
    bool generateHaptics = false;

    if (auto slider = m_strokeWidthSlider.get())
    {
        slider.as<winrt::Control>().Focus(winrt::FocusState::Keyboard);

        double minValue = slider.Minimum();
        double maxValue = slider.Maximum();
        double value = slider.Value();

        double newValue = forward ? (std::min)(maxValue, value + 1.0) : (std::max)(minValue, value - 1.0);

        // Disable haptics at either end.
        generateHaptics = (!AreSame(newValue, minValue)) && (!AreSame(newValue, maxValue));

        slider.Value(newValue);
    }

    return generateHaptics;
}

// Lightweight 2D pen-flyout preview: a Path (sample stroke) whose color + thickness track the current pen.
// UWP hosted a live mini-InkPresenter here (IInkPresenterHost/ElementCompositionPreview), which the lift
// does not surface; a Path gives the same "changes with color/size" affordance.
void InkToolbarPenConfigurationControl::ConfigureStrokeWidthPreview()
{
    namespace mux = winrt::Microsoft::UI::Xaml;

    auto canvas = GetTemplateChild(L"StrokePreviewCanvas").try_as<mux::Controls::Canvas>();
    auto grid = GetTemplateChild(L"StrokePreviewGrid").try_as<mux::Controls::Grid>();
    if (!canvas || !grid)
    {
        return;
    }
    m_strokePreviewGrid = winrt::make_weak(grid);

    auto figure = mux::Media::PathFigure();
    figure.StartPoint(winrt::Windows::Foundation::Point{
        s_previewStrokeCoordinates[0], s_previewStrokeCoordinates[1] });

    auto segment = mux::Media::PolyLineSegment();
    auto points = segment.Points();
    float minX = s_previewStrokeCoordinates[0];
    float maxX = minX;
    float minY = s_previewStrokeCoordinates[1];
    float maxY = minY;
    for (size_t i = 2; i < _countof(s_previewStrokeCoordinates); i += 2)
    {
        const float x = s_previewStrokeCoordinates[i];
        const float y = s_previewStrokeCoordinates[i + 1];
        points.Append(winrt::Windows::Foundation::Point{ x, y });
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }
    figure.Segments().Append(segment);

    auto geometry = mux::Media::PathGeometry();
    geometry.Figures().Append(figure);

    auto path = mux::Shapes::Path();
    path.Data(geometry);
    path.StrokeStartLineCap(mux::Media::PenLineCap::Round);
    path.StrokeEndLineCap(mux::Media::PenLineCap::Round);
    path.StrokeLineJoin(mux::Media::PenLineJoin::Round);
    path.RenderTransform(mux::Media::TranslateTransform());
    canvas.Children().Append(path);
    m_previewPath = winrt::make_weak(path);

    m_previewGeometryBounds = winrt::Windows::Foundation::Rect{
        minX, minY, maxX - minX, maxY - minY };

    m_previewGridSizeChangedToken = grid.SizeChanged({ this, &InkToolbarPenConfigurationControl::OnPreviewGridSizeChanged });

    UpdatePreview();
}

// Push the current pen color + width onto the preview stroke; highlighter renders translucent.
void InkToolbarPenConfigurationControl::UpdatePreview()
{
    auto path = m_previewPath.get();
    if (!path)
    {
        return;
    }

    auto toolbar = m_inkToolbar.get();
    if (!toolbar)
    {
        return;
    }

    auto attributes = toolbar.InkDrawingAttributes();
    if (!attributes)
    {
        return;
    }

    path.Stroke(winrt::SolidColorBrush(attributes.Color()));
    path.StrokeThickness(attributes.Size().Width);
    path.Opacity(PenButton().try_as<winrt::Microsoft::UI::Xaml::Controls::InkToolbarHighlighterButton>() ? 0.5 : 1.0);

    PositionPreviewStroke();
}

// Center the sample stroke in the preview canvas, sizing the canvas for the widest stroke so the layout
// does not jump while the width slider is dragged.
void InkToolbarPenConfigurationControl::PositionPreviewStroke()
{
    auto path = m_previewPath.get();
    auto canvas = GetTemplateChild(L"StrokePreviewCanvas").try_as<winrt::Microsoft::UI::Xaml::Controls::Canvas>();
    auto grid = m_strokePreviewGrid.get();
    if (!path || !canvas || !grid)
    {
        return;
    }

    double maxThickness = path.StrokeThickness();
    if (auto button = PenButton())
    {
        maxThickness = std::max(maxThickness, button.MaxStrokeWidth());
    }

    const double canvasHeight = m_previewGeometryBounds.Height + maxThickness + c_previewStrokeMargin;
    canvas.Height(canvasHeight);

    double canvasWidth = canvas.ActualWidth();
    if (canvasWidth <= 0.0)
    {
        canvasWidth = grid.ActualWidth();
    }

    if (auto transform = path.RenderTransform().try_as<winrt::Microsoft::UI::Xaml::Media::TranslateTransform>())
    {
        const double midY = m_previewGeometryBounds.Y + m_previewGeometryBounds.Height / 2.0;
        transform.Y(canvasHeight / 2.0 - midY);
        if (canvasWidth > 0.0)
        {
            transform.X(0.5 * (canvasWidth - m_previewGeometryBounds.Width) - m_previewGeometryBounds.X);
        }
    }
}

void InkToolbarPenConfigurationControl::OnPreviewGridSizeChanged(winrt::IInspectable const& /*sender*/, winrt::SizeChangedEventArgs const& /*args*/)
{
    PositionPreviewStroke();
}

bool InkToolbarPenConfigurationControl::IsHighContrast() const
{
    try
    {
        return m_accessibilitySettings && m_accessibilitySettings.HighContrast();
    }
    catch (winrt::hresult_error const& e)
    {
        InkToolbarLogHResult(e.code(), L"high-contrast state query");
        return false;
    }
}

// Wire AccessibilitySettings so the color flyout swaps to the high-contrast palette when HC toggles.
void InkToolbarPenConfigurationControl::ConfigureHighContrast()
{
    try
    {
        if (!m_accessibilitySettings)
        {
            m_accessibilitySettings = winrt::Windows::UI::ViewManagement::AccessibilitySettings();
        }
        m_highContrastChangedToken = m_accessibilitySettings.HighContrastChanged(
            { this, &InkToolbarPenConfigurationControl::OnHighContrastChanged });
    }
    catch (winrt::hresult_error const& e)
    {
        InkToolbarLogHResult(e.code(), L"high-contrast monitoring setup");
    }

    // Reflect the current HC state immediately.
    RegenerateItemSource();
}

void InkToolbarPenConfigurationControl::OnHighContrastChanged(
    winrt::Windows::UI::ViewManagement::AccessibilitySettings const& /*sender*/, winrt::IInspectable const& /*args*/)
{
    // HighContrastChanged can arrive off the UI thread; marshal the flyout refresh back.
    if (auto dispatcher = DispatcherQueue())
    {
        auto strongThis = get_strong();
        dispatcher.TryEnqueue([strongThis]()
        {
            strongThis->RegenerateItemSource();
            strongThis->UpdatePreview();
        });
    }
}

// STUB: the flyout background brush is only needed to composite the highlighter preview background stroke,
// which is part of the (unavailable) wincomp preview. No-op.
void InkToolbarPenConfigurationControl::UpdateCurrentFlyoutBackgroundBrush()
{
}
