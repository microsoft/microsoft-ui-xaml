// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlAmbientLight.h"
#include "IsTargetDPHelper.h"

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "Microsoft.UI.Private.Composition.Effects_impl.h"
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
#if BUILD_WINDOWS
    if (!m_materialProperties)
    {
        m_materialProperties = winrt::MaterialProperties::GetForCurrentView();

        // Dispatcher needed as TransparencyPolicyChanged is raised off thread
        if (!m_dispatcherQueue)
        {
            m_dispatcherQueue = winrt::DispatcherQueue::GetForCurrentThread();
        }

        // We might have no dispatcher in XamlPresenter scenarios (currenlty LogonUI/CredUI do not appear to use Acrylic).
        // In these cases, we will honor the initial policy state but not get change notifications.
        // This matches the legacy MaterialHelper behavior and should be sufficient for the special case of login screen.
        if (m_dispatcherQueue)
        {
            m_transparencyPolicyChangedRevoker = m_materialProperties.TransparencyPolicyChanged(winrt::auto_revoke, {
                [weakThis = get_weak(), dispatcherQueue = m_dispatcherQueue] (const winrt::IMaterialProperties& sender, const winrt::IInspectable& args)
                {
                    MaterialHelper::LightTemplates<XamlAmbientLight>::OnLightTransparencyPolicyChanged(
                        weakThis,
                        sender,
                        dispatcherQueue,
                        false /* onUIThread */);
                }
                });
        }
    }

    // Apply Initial policy state
    MaterialHelper::LightTemplates<XamlAmbientLight>::OnLightTransparencyPolicyChanged(
        get_weak(),
        m_materialProperties,
        m_dispatcherQueue,
        true /* onUIThread */);
    m_additionalMaterialPolicyChangedToken = MaterialHelper::AdditionalPolicyChanged([this](auto sender) { OnAdditionalMaterialPolicyChanged(sender); });
#else
    m_materialPolicyChangedToken = MaterialHelper::PolicyChanged([this](auto sender, auto args) { OnMaterialPolicyStatusChanged(sender, args); });
#endif
    if (!m_isDisabledByMaterialPolicy)
    {
        EnsureCompositionResources();
    }

}

void XamlAmbientLight::OnDisconnected(winrt::UIElement const& /*oldElement*/)
{
    ReleaseCompositionResources();

#if BUILD_WINDOWS
    MaterialHelper::AdditionalPolicyChanged(m_additionalMaterialPolicyChangedToken);
    m_additionalMaterialPolicyChangedToken.value = 0;
#else
    MaterialHelper::PolicyChanged(m_materialPolicyChangedToken);
    m_materialPolicyChangedToken.value = 0;
#endif
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

#if BUILD_WINDOWS
void XamlAmbientLight::OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender)
{
    MaterialHelper::LightTemplates<XamlAmbientLight>::OnLightTransparencyPolicyChanged(
        get_weak(),
        m_materialProperties,
        m_dispatcherQueue,
        true /* onUIThread */);
}
#else
void XamlAmbientLight::OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy)
{
    MaterialHelper::LightPolicyChangedHelper<XamlAmbientLight>(this, isDisabledByMaterialPolicy);
}
#endif

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
