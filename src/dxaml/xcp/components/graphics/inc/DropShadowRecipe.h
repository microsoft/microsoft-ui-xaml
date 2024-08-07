// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <RuntimeEnabledFeatures.h>

// The values necessary to achieve the theme shadow look, intended to be applied to a composition drop shadow.
struct DropShadowRecipe
{
    // Stores the corner radiuses of the content rounded rect that's casting the shadow.
    XFLOAT RadiusX;
    XFLOAT RadiusY;

    // Drop shadows have the concept of elevation, which effectively simulates elevation that projected shadows
    // have (greater elevation = bigger shadow).
    float Elevation;

    // There are two sets of BlurRadius/Color because we'll be creating the DropShadowVisual by combining two
    // visuals - an Ambient and a Directional visual with drop shadows.

    // Provided by design. Determines how "blurred" the DropShadow should be and how far out it extends.
    float AmbientBlurRadius;
    float DirectionalBlurRadius;

    // Provided by design. Shadows can have a Y offset to give the illusion that light is hitting the caster at a certain angle.
    float AmbientYOffset;
    float DirectionalYOffset;

    // Provided by design. The opacity of the shadow.
    XFLOAT AmbientOpacity;
    XFLOAT DirectionalOpacity;

    // The color of the drop shadow being casted. The AmbientOpacity/DirectionalOpacity is already multiplied in.
    wu::Color AmbientColor;
    wu::Color DirectionalColor;

    // How much the drop shadow should poke out from underneath the caster.
    XTHICKNESS Insets;

    bool operator<(const DropShadowRecipe& o) const
    {
        return std::tie(RadiusX,   RadiusY,   Elevation,   AmbientOpacity,   DirectionalOpacity) <
             std::tie(o.RadiusX, o.RadiusY, o.Elevation, o.AmbientOpacity, o.DirectionalOpacity);
    }
};

// The "theme" drop shadow actually consists of two drop shadows, designed to simulate shadows generated by
// two shell-wide light sources: one ambient, and one directional.
// These formulas were taken from the shell team implementation of the ThemeNineGridShadowBrush
// At low elevations [2 to 16], there should not be an Ambient shadow.
// At high elevations (> 16), there will be an Ambient shadow.
inline DropShadowRecipe GetDropShadowRecipe(float translationZ, const DirectUI::ElementTheme t)
{
    //
    // An element is drawn with an ambient shadow and an offsetted directional shadow.
    // Here are the visuals that we produce:
    //
    //     ,---,--ambient-shadow--.---.  
    //    /   / ,----------------. \   \  
    //   /   | |                  | |   \ 
    //  |    | |                  | |    |
    //  |    | |                  | |    |
    //  |    | |      hollow      | |    |
    //  |    | |      element     | |    |
    //  |    | |                  | |    |
    //  |    | |                  | |    |
    //  |    | |                  | |    |
    //  |     \ `----------------' /     |
    //  |      `------------------'      |
    //  |                                |
    //   \      directional shadow      /
    //    \                            /
    //     `--------------------------'
    //
    // We draw these visuals, then put the surface as the source of a ninegrid brush:
    //
    //           I              I
    //     ,---,-I--------------I-.---.  
    //    /   / ,I--------------I. \   \  
    //   /   | | I              I | |   \ 
    // ==========+==============+==========
    //  |    | | I              I | |    |
    //  |    | | I              I | |    |
    //  |    | | I    hollow    I | |    |
    //  |    | | I              I | |    |
    //  |    | | I              I | |    |
    // ==========+==============+==========
    //  |     \ `I--------------I' /     |
    //  |      `-I--------------I-'      |
    //  |        I              I        |
    //   \       I              I       /
    //    \      I              I      /
    //     `-----I--------------I-----'
    //           I              I
    //
    // We leave enough room in the outside cells of the nine grid to draw the shadow's pixels. Note that the nine
    // grid thicknesses must account for both the amount of shadow as well as the rounded corners of the content
    // itself. Here we calculate the amount of space needed for the shadow.
    //
    // Note that the real straightforward approach is to just put a CompositionDropShadow on the visual that wants
    // the drop shadow, but this creates problems around clips. Xaml sets up a visual tree that can contain many
    // clips, some of which we want to clip out the shadow and others which we do not. We don't have a way of
    // specifying which clips should affect the CompositionDropShadow which way, so we just draw the drop shadow
    // separately and put it into the Xaml visual tree ourselves as if it was an image.
    //

    DropShadowRecipe recipe{};
    recipe.Elevation = std::min(64.0f, translationZ / 2);   // Clamped to a max of 64 (corresponding to Translation.Z = 128)

    if (recipe.Elevation < 2)
    {
        recipe.AmbientBlurRadius = 2.0f;

        recipe.AmbientOpacity = 0;
        recipe.DirectionalOpacity = 0;
    }
    else if (recipe.Elevation >= 2 && recipe.Elevation <= 16)
    {
        recipe.AmbientBlurRadius = 2.0f;

        // Ambient shadow won't show.
        recipe.AmbientOpacity = 0;
        if (t == DirectUI::ElementTheme::Light)
        {
            recipe.DirectionalOpacity = std::min((recipe.Elevation / 100.0f) + 0.06f, 0.14f);   // maxes out at elevation = 8
        }
        else if (t == DirectUI::ElementTheme::Dark)
        {
            recipe.DirectionalOpacity = 0.26f;
        }
    }
    else
    {
        recipe.AmbientBlurRadius = recipe.Elevation / 3;    // Translation.Z / 6, but we already divided by 2
        recipe.AmbientYOffset = 2;

        if (t == DirectUI::ElementTheme::Light)
        {
            recipe.AmbientOpacity = 0.15f;
            recipe.DirectionalOpacity = 0.19f;
        }
        else if (t == DirectUI::ElementTheme::Dark)
        {
            recipe.AmbientOpacity = 0.37f;
            recipe.DirectionalOpacity = 0.37f;
        }
    }

    recipe.AmbientColor = wu::Color{static_cast<uint8_t>(recipe.AmbientOpacity * 255), 0, 0, 0};
    recipe.DirectionalBlurRadius = recipe.Elevation;
    recipe.DirectionalYOffset = recipe.Elevation * 0.5f;   // positive means shifted down
    recipe.DirectionalColor = wu::Color{static_cast<uint8_t>(recipe.DirectionalOpacity * 255), 0, 0, 0};

    const float maxBlurRadius = std::ceilf(std::max(recipe.AmbientBlurRadius, recipe.DirectionalBlurRadius));
    recipe.Insets.left = maxBlurRadius;
    recipe.Insets.right = maxBlurRadius;
    // If the shadow is shifted vertically, then the amount of space needed for the shadow at the top and bottom
    // depend on both the shadow's blur radius and how much it shifted. A positive offset means to shift down, so
    // subtract from the top and add to the bottom.
    recipe.Insets.top = std::ceilf(std::max(recipe.AmbientBlurRadius - recipe.AmbientYOffset, recipe.DirectionalBlurRadius - recipe.DirectionalYOffset));
    recipe.Insets.bottom = std::ceilf(std::max(recipe.AmbientBlurRadius + recipe.AmbientYOffset, recipe.DirectionalBlurRadius + recipe.DirectionalYOffset));

    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableDropShadowDebugVisual))
    {
        // Debugging aid:  Draw the shadows in green if reg-key is set.  Helps with visibility.
        recipe.AmbientColor = wu::Color{static_cast<uint8_t>(255), 0, 255, 0};
        recipe.DirectionalColor = wu::Color{static_cast<uint8_t>(255), 0, 255, 0};
    }

    return recipe;
}
