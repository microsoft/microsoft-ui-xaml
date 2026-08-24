// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarPenConfigurationControl_Partial.cpp.
// PORTABLE logic preserved 1:1: color picker (PenColorPalette ListViewBase), stroke-width Slider
// (PenStrokeWidthSlider/EraserStrokeWidthSlider), selection <-> pen SelectedBrushIndex, slider <-> pen
// SelectedStrokeWidth, focus + keyboard/dial navigation, ColorNames tooltips, item source from the pen
// Palette. STUBBED + documented (lift-unavailable, subagent-verified): UpdatePreview / preview stroke /
// UpdateCurrentFlyoutBackgroundBrush (IInkPresenterHost + ElementCompositionPreview), Surface Dial
// (ExternalActorMode/Refocus), high-contrast palette + monitoring (AccessibilitySettings). The pen L3
// XAML template ("InkToolbarPenConfigurationControl" style) must be ported for the parts to resolve.

#include "pch.h"
#include "common.h"
#include "InkToolbarPenConfigurationControl.h"
#include "InkToolbar.h"

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
    // ConfigureStrokeWidthPreview: STUB - live wincomp stroke preview is not portable to the lift.

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
    // ConfigureHighContrast: STUB - AccessibilitySettings high-contrast monitoring is not available in lift.
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
        palette = button.Palette();
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

// STUB: live wincomp stroke preview requires IInkPresenterHost + ElementCompositionPreview, which are not
// available in the WinUI 3 lift (subagent-verified). File a platform bug to expose a preview surface, or
// replace with a lightweight 2D preview. No-op for now so color/width selection still function.
void InkToolbarPenConfigurationControl::UpdatePreview()
{
}

// STUB: the flyout background brush is only needed to composite the highlighter preview background stroke,
// which is part of the (unavailable) wincomp preview. No-op.
void InkToolbarPenConfigurationControl::UpdateCurrentFlyoutBackgroundBrush()
{
}
