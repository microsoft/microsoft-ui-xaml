// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "MaterialHelper.h"
#include "RevealTestApi.h"

#include "RevealBorderLight.g.h"

class PointerHoverWindowHelper;
class SoftLightSwitch;

class RevealBorderLight :
    public ReferenceTracker<RevealBorderLight, winrt::implementation::RevealBorderLightT>
{
    friend RevealTestApi;
    friend MaterialHelperBase;
    friend MaterialHelper;
public:
    static winrt::hstring& GetLightThemeIdStatic();
    static winrt::hstring& GetDarkThemeIdStatic();

    void SetIsWideLight(bool isWideLight) { m_isWideLight = isWideLight; }
    void SetIsLightTheme(bool isLightTheme) { m_isLightTheme = isLightTheme; }

    // IXamlLightOverrides
    winrt::hstring GetId();
    void OnConnected(winrt::UIElement const& newElement);
    void OnDisconnected(winrt::UIElement const& oldElement);
    void OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy);

private:
    winrt::SpotLight GetLight() { return m_compositionSpotLight; } // For test APIs
    void HookupWindowPointerHandlers();
    void UnhookWindowPointerHandlers();
    void EnsureCompositionResources();
    void ReleaseCompositionResources();
    bool GetShouldLightBeOn();
    void SwitchLight(bool turnOn);

    void EnsureLocalLight();
    void ReleaseLocalLight();

    winrt::weak_ref<winrt::UIElement> m_targetElement{ nullptr };
    winrt::ExpressionAnimation m_offsetAnimation{ nullptr };
    winrt::CompositionPropertySet m_pointer{ nullptr };
    winrt::SpotLight m_compositionSpotLight{ nullptr };
    winrt::CompositionPropertySet m_colorsProxy{ nullptr };
    winrt::CompositionPropertySet m_offsetProps{ nullptr };

    winrt::event_token m_PointerEnteredToken{};
    winrt::event_token m_PointerExitedToken{};
    winrt::event_token m_PointerMovedToken{};
    winrt::event_token m_PointerPressedToken{};
    winrt::event_token m_PointerReleasedToken{};
    winrt::event_token m_PointerCaptureLostToken{};
    bool m_gotPointerCaptureLostDueToDManip{};

    bool m_isWideLight{};
    bool m_isLightTheme{};

    bool m_isDisabledByMaterialPolicy{};

    winrt::event_token m_materialPolicyChangedToken{};
    bool m_shouldLightBeOn{};
    bool m_setLightDisabledAfterTurnOffAnimation{ true };

    winrt::CoreWindow m_coreWindow{ nullptr };
};
