#include "pch.h"
#include "common.h"
#include "SystemBackdropBrushFactory.h"


namespace SystemBackdropComponentInternal
{
    winrt::CompositionBrush BuildMicaEffectBrush(const winrt::Compositor& compositor, winrt::Windows::UI::Color tintColor, float tintOpacity, float luminosityOpacity)
    {
        // Tint Color.
        auto tintColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
        tintColorEffect->Name(L"TintColor");
        tintColorEffect->Color(tintColor);

        // OpacityEffect applied to Tint.
        auto tintOpacityEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::OpacityEffect>();
        tintOpacityEffect->Name(L"TintOpacity");
        tintOpacityEffect->Opacity(tintOpacity);
        tintOpacityEffect->Source(*tintColorEffect);

        // Apply Luminosity:

        // Luminosity Color.
        auto luminosityColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
        luminosityColorEffect->Color(tintColor);

        // OpacityEffect applied to Luminosity.
        auto luminosityOpacityEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::OpacityEffect>();
        luminosityOpacityEffect->Name(L"LuminosityOpacity");
        luminosityOpacityEffect->Opacity(luminosityOpacity);
        luminosityOpacityEffect->Source(*luminosityColorEffect);

        // Luminosity Blend.
        // NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
        auto luminosityBlendEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::BlendEffect>();
        luminosityBlendEffect->Mode(winrt::BlendEffectMode::Color);
        luminosityBlendEffect->Background(winrt::CompositionEffectSourceParameter{ L"BlurredWallpaperBackdrop" });
        luminosityBlendEffect->Foreground(*luminosityOpacityEffect);

        // Apply Tint:

        // Color Blend.
        // NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
        auto colorBlendEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::BlendEffect>();
        colorBlendEffect->Mode(winrt::BlendEffectMode::Luminosity);
        colorBlendEffect->Background(*luminosityBlendEffect);
        colorBlendEffect->Foreground(*tintOpacityEffect);

        winrt::CompositionEffectBrush micaEffectBrush = compositor.CreateEffectFactory(*colorBlendEffect).CreateBrush();
        micaEffectBrush.SetSourceParameter(L"BlurredWallpaperBackdrop", compositor.TryCreateBlurredWallpaperBackdropBrush());

        return micaEffectBrush;
    }

    winrt::CompositionBrush CreateCrossFadeEffectBrush(const winrt::Compositor& compositor, const winrt::CompositionBrush& from, const winrt::CompositionBrush& to)
    {
        auto crossFadeEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::CrossFadeEffect>();
        crossFadeEffect->Name(L"Crossfade"); // Name to reference when starting the animation.
        crossFadeEffect->Source1(winrt::CompositionEffectSourceParameter{ L"source1" });
        crossFadeEffect->Source2(winrt::CompositionEffectSourceParameter{ L"source2" });
        crossFadeEffect->Weight(0);

        winrt::CompositionEffectBrush crossFadeEffectBrush = compositor.CreateEffectFactory(*crossFadeEffect, { L"Crossfade.Weight" }).CreateBrush();
        crossFadeEffectBrush.Comment(L"Crossfade");
        crossFadeEffectBrush.SetSourceParameter(L"source1", from);
        crossFadeEffectBrush.SetSourceParameter(L"source2", to);
        return crossFadeEffectBrush;
    }

    winrt::ScalarKeyFrameAnimation CreateCrossFadeAnimation(const winrt::Compositor& compositor)
    {
        winrt::ScalarKeyFrameAnimation animation = compositor.CreateScalarKeyFrameAnimation();
        winrt::LinearEasingFunction linearEasing = compositor.CreateLinearEasingFunction();
        animation.InsertKeyFrame(0.0f, 0.0f, linearEasing);
        animation.InsertKeyFrame(1.0f, 1.0f, linearEasing);
        animation.Duration(std::chrono::milliseconds{ 250 });
        return animation;
    }
}
