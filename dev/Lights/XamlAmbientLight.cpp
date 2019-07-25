// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlAmbientLight.h"
#include "IsTargetDPHelper.h"

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "Microsoft.UI.Composition.Effects_impl.h"
#pragma warning(pop)

const winrt::Color XamlAmbientLight::sc_defaultColor{ 255, 255, 255, 255 };

XamlAmbientLight::XamlAmbientLight()
{
    m_ambientLightColor = sc_defaultColor;
}

winrt::hstring& XamlAmbientLight::GetLightIdStatic()
{
    static winrt::hstring s_XamlAmbientLightId{ MUXCONTROLSMEDIA_NAMESPACE_STR L".XamlAmbientLight" };
    return s_XamlAmbientLightId;
}

winrt::hstring XamlAmbientLight::GetId()
{
    return GetLightIdStatic();
}

void XamlAmbientLight::OnConnected(winrt::UIElement const& /*newElement*/)
{
    m_materialPolicyChangedToken = MaterialHelper::PolicyChanged([this](auto sender, auto args) { OnMaterialPolicyStatusChanged(sender, args); });
    if (!m_isDisabledByMaterialPolicy)
    {
        EnsureCompositionResources();
    }

}

void XamlAmbientLight::OnDisconnected(winrt::UIElement const& /*oldElement*/)
{
    ReleaseCompositionResources();
    MaterialHelper::PolicyChanged(m_materialPolicyChangedToken);
    m_materialPolicyChangedToken.value = 0;
}

void XamlAmbientLight::EnsureCompositionResources()
{
    if (!m_compositionAmbientLight)
    {
        auto compositor = winrt::Window::Current().Compositor();
        m_compositionAmbientLight = compositor.CreateAmbientLight();
        m_compositionAmbientLight.Color(m_ambientLightColor);
        CompositionLight(m_compositionAmbientLight);
    }
}

void XamlAmbientLight::ReleaseCompositionResources()
{
    if (m_compositionAmbientLight)
    {
        m_compositionAmbientLight = nullptr;
        CompositionLight(nullptr);
    }
}

void XamlAmbientLight::OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy)
{
    MaterialHelper::LightPolicyChangedHelper<XamlAmbientLight>(this, isDisabledByMaterialPolicy);
}

void XamlAmbientLight::OnColorPropertyChanged(
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    m_ambientLightColor = unbox_value<winrt::Color>(args.NewValue());
    if (m_compositionAmbientLight)
    {
        m_compositionAmbientLight.Color(m_ambientLightColor);
    }
}

void XamlAmbientLight::OnIsTargetPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    OnAttachedIsTargetPropertyChanged<XamlAmbientLight>(sender, args);
}
