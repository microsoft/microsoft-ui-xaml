// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BackdropMaterialTestApi.g.h"

#include "BackdropMaterial.h"
#include "MicaController.h"

class BackdropMaterialTestApi : public winrt::implementation::BackdropMaterialTestApiT<BackdropMaterialTestApi>
{
public:
    static auto GetMicaController() { return BackdropMaterial::GetMicaController(); }

    static bool IsBackdropMaterialActive()
    {
        return static_cast<bool>(GetMicaController());
    }

    static winrt::Color TintColor()
    {
        return GetMicaController() ? GetMicaController()->TintColor() : winrt::Color{0,0,0,0};
    }
    static void TintColor(winrt::Color const& value)
    {
        if (GetMicaController())
        {
            GetMicaController()->TintColor(value);
        }
    }
    static float TintOpacity()
    {
        return GetMicaController()->TintOpacity();
    }
    static void TintOpacity(float value)
    {
        if (GetMicaController())
        {
            GetMicaController()->TintOpacity(value);
        }
    }
    static float LuminosityOpacity()
    {
        return GetMicaController()->LuminosityOpacity();
    }
    static void LuminosityOpacity(float value)
    {
        if (GetMicaController())
        {
            GetMicaController()->LuminosityOpacity(value);
        }
    }
    static winrt::Color FallbackColor()
    {
        return GetMicaController() ? GetMicaController()->FallbackColor() : winrt::Color{ 0,0,0,0 };
    }
    static void FallbackColor(winrt::Color const& value)
    {
        if (GetMicaController())
        {
            GetMicaController()->FallbackColor(value);
        }
    }

};

struct BackdropMaterialTestApiFactory : winrt::factory_implementation::BackdropMaterialTestApiT<BackdropMaterialTestApiFactory, BackdropMaterialTestApi>
{
};

namespace winrt::Microsoft::UI::Private::Controls
{
    namespace factory_implementation { using BackdropMaterialTestApi = ::BackdropMaterialTestApiFactory; };
    namespace implementation { using BackdropMaterialTestApi = ::BackdropMaterialTestApi; };
}
