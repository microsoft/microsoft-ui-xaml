// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ColorHelpers.h"
#include "ColorChangedEventArgs.h"
#include "DispatcherHelper.h"

#include "ColorSpectrum.g.h"
#include "ColorSpectrum.properties.h"

class ColorSpectrum :
    public ReferenceTracker<ColorSpectrum, winrt::implementation::ColorSpectrumT>,
    public ColorSpectrumProperties
{
public:
    ColorSpectrum();

    // IUIElementOverridesHelper overrides
    winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElementOverrides overrides
    void OnApplyTemplate();

    // IControlOverrides overrides
    void OnKeyDown(winrt::KeyRoutedEventArgs const& e);
    void OnGotFocus(winrt::RoutedEventArgs const& e);
    void OnLostFocus(winrt::RoutedEventArgs const& e);

    // Property changed handler.
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    winrt::Rect GetBoundingRectangle();
    void RaiseColorChanged();

private:

    // DependencyProperty changed event handlers
    void OnColorChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnHsvColorChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnMinMaxHueChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnMinMaxSaturationChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnMinMaxValueChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnShapeChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnComponentsChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // ColorSpectrum event handlers
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    // Template part event handlers
    void OnLayoutRootSizeChanged(winrt::IInspectable const& sender, winrt::SizeChangedEventArgs const& args);
    void OnInputTargetPointerEntered(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnInputTargetPointerExited(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnInputTargetPointerMoved(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnInputTargetPointerReleased(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnInputTargetPointerPressed(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnSelectionEllipseFlowDirectionChanged(winrt::DependencyObject const& o, winrt::DependencyProperty const& p);

    // Helper functions
    void SetColor();

    void UpdateVisualState(bool useTransitions);
    void UpdateColor(Hsv newHsv);
    void UpdateColorFromPoint(const winrt::PointerPoint& point);
    void UpdateEllipse();

    void CreateBitmapsAndColorMap();
    void UpdateBitmapSources();

    bool SelectionEllipseShouldBeLight();

    // Helpers used by CreateBitmapsAndColorMap() to fill pixel data and create bitmaps from that data.
    static void FillPixelForBox(
        double x,
        double y,
        const Hsv &baseHsv,
        double minDimension,
        winrt::ColorSpectrumComponents components,
        double minHue,
        double maxHue,
        double minSaturation,
        double maxSaturation,
        double minValue,
        double maxValue,
        std::shared_ptr<std::vector<byte>> bgraMinPixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle1PixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle2PixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle3PixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle4PixelData,
        std::shared_ptr<std::vector<byte>> bgraMaxPixelData,
        std::shared_ptr<std::vector<Hsv>> newHsvValues);
    static void FillPixelForRing(
        double x,
        double y,
        double radius,
        const Hsv &baseHsv,
        winrt::ColorSpectrumComponents components,
        double minHue,
        double maxHue,
        double minSaturation,
        double maxSaturation,
        double minValue,
        double maxValue,
        std::shared_ptr<std::vector<byte>> bgraMinPixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle1PixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle2PixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle3PixelData,
        std::shared_ptr<std::vector<byte>> bgraMiddle4PixelData,
        std::shared_ptr<std::vector<byte>> bgraMaxPixelData,
        std::shared_ptr<std::vector<Hsv>> newHsvValues);

    bool m_updatingColor;
    bool m_updatingHsvColor;
    bool m_isPointerOver;
    bool m_isPointerPressed;
    bool m_shouldShowLargeSelection;
    std::vector<Hsv> m_hsvValues;

    // XAML elements
    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::Grid> m_sizingGrid{ this };

    tracker_ref<winrt::Rectangle> m_spectrumRectangle{ this };
    tracker_ref<winrt::Ellipse> m_spectrumEllipse{ this };
    tracker_ref<winrt::Rectangle> m_spectrumOverlayRectangle{ this };
    tracker_ref<winrt::Ellipse> m_spectrumOverlayEllipse{ this };

    tracker_ref<winrt::FrameworkElement> m_inputTarget{ this };
    tracker_ref<winrt::Panel> m_selectionEllipsePanel{ this };

    tracker_ref<winrt::ToolTip> m_colorNameToolTip{ this };

    winrt::IAsyncAction m_createImageBitmapAction{ nullptr };

    // On RS1 and before, we put the spectrum images in a bitmap,
    // which we then give to an ImageBrush.
    winrt::WriteableBitmap m_hueRedBitmap{ nullptr };
    winrt::WriteableBitmap m_hueYellowBitmap{ nullptr };
    winrt::WriteableBitmap m_hueGreenBitmap{ nullptr };
    winrt::WriteableBitmap m_hueCyanBitmap{ nullptr };
    winrt::WriteableBitmap m_hueBlueBitmap{ nullptr };
    winrt::WriteableBitmap m_huePurpleBitmap{ nullptr };

    winrt::WriteableBitmap m_saturationMinimumBitmap{ nullptr };
    winrt::WriteableBitmap m_saturationMaximumBitmap{ nullptr };

    winrt::WriteableBitmap m_valueBitmap{ nullptr };

    // On RS2 and later, we put the spectrum images in a loaded image surface,
    // which we then put into a SpectrumBrush.
    winrt::LoadedImageSurface m_hueRedSurface{ nullptr };
    winrt::LoadedImageSurface m_hueYellowSurface{ nullptr };
    winrt::LoadedImageSurface m_hueGreenSurface{ nullptr };
    winrt::LoadedImageSurface m_hueCyanSurface{ nullptr };
    winrt::LoadedImageSurface m_hueBlueSurface{ nullptr };
    winrt::LoadedImageSurface m_huePurpleSurface{ nullptr };

    winrt::LoadedImageSurface m_saturationMinimumSurface{ nullptr };
    winrt::LoadedImageSurface m_saturationMaximumSurface{ nullptr };

    winrt::LoadedImageSurface m_valueSurface{ nullptr };

    // Fields used by UpdateEllipse() to ensure that it's using the data
    // associated with the last call to CreateBitmapsAndColorMap(),
    // in order to function properly while the asynchronous bitmap creation
    // is in progress.
    winrt::ColorSpectrumShape m_shapeFromLastBitmapCreation{ winrt::ColorSpectrumShape::Box };
    winrt::ColorSpectrumComponents m_componentsFromLastBitmapCreation{ winrt::ColorSpectrumComponents::HueSaturation };
    double m_imageWidthFromLastBitmapCreation{ 0.0 };
    double m_imageHeightFromLastBitmapCreation{ 0.0 };
    int m_minHueFromLastBitmapCreation{ 0 };
    int m_maxHueFromLastBitmapCreation{ 0 };
    int m_minSaturationFromLastBitmapCreation{ 0 };
    int m_maxSaturationFromLastBitmapCreation{ 0 };
    int m_minValueFromLastBitmapCreation{ 0 };
    int m_maxValueFromLastBitmapCreation{ 0 };

    winrt::Color m_oldColor{ 255, 255, 255, 255 };
    winrt::float4 m_oldHsvColor{ 0.0, 0.0, 1.0, 1.0 };

    DispatcherHelper m_dispatcherHelper{ *this };
};