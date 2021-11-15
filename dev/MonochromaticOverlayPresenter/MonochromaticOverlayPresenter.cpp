// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MonochromaticOverlayPresenter.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

struct XamlCompBrush : winrt::Windows::UI::Xaml::Media::XamlCompositionBrushBaseT<XamlCompBrush>
{
    void SetCompositionBrush(winrt::CompositionBrush const& brush)
    {
        CompositionBrush(brush);
    }
};

MonochromaticOverlayPresenter::MonochromaticOverlayPresenter()
{
    SizeChanged([this](auto&&...) {
        InvalidateBrush();
        });
}

void MonochromaticOverlayPresenter::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    InvalidateBrush();
}

void MonochromaticOverlayPresenter::InvalidateBrush()
{
    // Delay brush updates until Tick to coalesce changes and avoid rebuilding the effect when we don't need to.
    if (!_needsBrushUpdate)
    {
        _needsBrushUpdate = true;
        SharedHelpers::QueueCallbackForCompositionRendering([weakThis = get_weak(), this]() {
            if (weakThis.get())
            {
                UpdateBrush();
            }
            _needsBrushUpdate = false;
        });
    }
}

void MonochromaticOverlayPresenter::UpdateBrush()
{
    if (auto targetElement = SourceElement())
    {
        auto const newColor = ReplacementColor();
        if (_replacementColor != newColor)
        {
            _replacementColor = newColor;
            _effectFactory = nullptr;
        }

        auto compositor = winrt::Window::Current().Compositor();

        if (!_effectFactory)
        {
            // Build an effect that takes the source image and uses the alpha channel and replaces all other channels with
            // the ReplacementColor's RGB.
            auto colorMatrixEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorMatrixEffect>();
            colorMatrixEffect->Source(winrt::CompositionEffectSourceParameter{ L"source" });
            winrt::Microsoft::UI::Private::Composition::Effects::Matrix5x4 colorMatrix = {};

            // If the ReplacementColor is not transparent then use the RGB values as the new color. Otherwise
            // just show the target by using an Identity colorMatrix.
            if (_replacementColor.A != 0)
            {
                colorMatrix.M51 = (float)((_replacementColor.R / 255.0));
                colorMatrix.M52 = (float)((_replacementColor.G / 255.0));
                colorMatrix.M53 = (float)((_replacementColor.B / 255.0));
                colorMatrix.M44 = 1;
            }
            else
            {
                colorMatrix.M11 = colorMatrix.M22 = colorMatrix.M33 = colorMatrix.M44 = 1;
            }
            colorMatrixEffect->ColorMatrix(colorMatrix);

            _effectFactory = compositor.CreateEffectFactory(*colorMatrixEffect);
        }

        auto const actualSize = winrt::float2{ (float)ActualWidth(), (float)ActualHeight() };
        auto transform = TransformToVisual(targetElement);
        auto const offset = transform.TransformPoint(winrt::Point{ 0, 0 });

        // Create a VisualSurface positioned at the same location as this control and feed that
        // through the color effect.
        auto surfaceBrush = compositor.CreateSurfaceBrush();
        surfaceBrush.Stretch(winrt::CompositionStretch::None);
        auto surface = compositor.CreateVisualSurface();

        // Select the source visual and the offset/size of this control in that element's space.
        surface.SourceVisual(winrt::ElementCompositionPreview::GetElementVisual(targetElement));
        surface.SourceOffset({ offset.X, offset.Y });
        surface.SourceSize(actualSize);
        surfaceBrush.Surface(surface);
        surfaceBrush.Stretch(winrt::CompositionStretch::None);
        auto compBrush = _effectFactory.CreateBrush();
        compBrush.SetSourceParameter(L"source", surfaceBrush);

        auto visual = compositor.CreateSpriteVisual();
        visual.Size(actualSize);
        visual.Brush(compBrush);

        winrt::ElementCompositionPreview::SetElementChildVisual(*this, visual);
    }

}
