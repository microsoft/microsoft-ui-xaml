// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "MaterialHelper.h"

#include "XamlAmbientLight.g.h"
#include "XamlAmbientLight.properties.h"

class XamlAmbientLight :
    public ReferenceTracker<XamlAmbientLight, winrt::implementation::XamlAmbientLightT>,
    public XamlAmbientLightProperties
{
    friend MaterialHelperBase;
    friend MaterialHelper;
public:
    XamlAmbientLight();

    static winrt::hstring& GetLightIdStatic();

    // IXamlLightOverrides
    winrt::hstring GetId();
    void OnConnected(winrt::UIElement const& newElement);
    void OnDisconnected(winrt::UIElement const& oldElement);

#if BUILD_WINDOWS
    void OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender);
#else
    void OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy);
#endif

    static void OnColorPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);
    static void OnIsTargetPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

public:
    static const winrt::Color sc_defaultColor;

private:
    void EnsureCompositionResources();
    void ReleaseCompositionResources();

    winrt::AmbientLight m_compositionAmbientLight{ nullptr };
    winrt::Color m_ambientLightColor{};

    bool m_isDisabledByMaterialPolicy{};
#if BUILD_WINDOWS
    winrt::DispatcherQueue m_dispatcherQueue{ nullptr };
    winrt::MaterialProperties m_materialProperties { nullptr };
    winrt::MaterialProperties::TransparencyPolicyChanged_revoker m_transparencyPolicyChangedRevoker{};
    winrt::event_token m_additionalMaterialPolicyChangedToken{};
#else
    winrt::event_token m_materialPolicyChangedToken{};
#endif
};
