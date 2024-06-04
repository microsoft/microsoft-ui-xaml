// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLight.h"
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <UIElement.h>
#include <DependencyObject.h>
#include <XamlLight.g.h>
#include <UIElement_Partial.h>
#include <xvector.h>

KnownTypeIndex CXamlLight::GetTypeIndex() const
{
    return DependencyObjectTraits<CXamlLight>::Index;
}

bool CXamlLight::HasWUCLight() const
{
    return m_wucLight != nullptr;
}

wrl::ComPtr<WUComp::ICompositionLight> CXamlLight::GetWUCLight()
{
    return m_wucLight;
}

_Check_return_ HRESULT CXamlLight::HandleROEClosed(HRESULT hr)
{
    if (hr == RO_E_CLOSED)
    {
        m_wucLight.Reset();
        m_wucLightTargets.Reset();

        // Also let all targeted UIElements know that this light is no longer targeting them. They'll propagate that
        // information to the light target map.
        for (const auto& pair : m_targetedVisuals)
        {
            pair.first->UntargetByLight(this);
        }
        m_targetedVisuals.clear();

        m_wasWUCLightClosed = true;

        return S_OK;
    }
    else
    {
        return hr;
    }
}

void CXamlLight::SetWUCLight(WUComp::ICompositionLight* wucLight)
{
    if (wucLight != m_wucLight.Get())
    {
        // If there's an old WUC light, untarget it first.
        if (m_wucLight != nullptr)
        {
            RemoveAllTargetElements();
            SetCoordinateSpace(nullptr);
            m_wucLight = nullptr;
            m_wucLightTargets = nullptr;
        }

        m_wucLight = wucLight;
        m_wasWUCLightClosed = false;

        if (m_wucLight != nullptr)
        {
            HRESULT hr = m_wucLight->get_Targets(m_wucLightTargets.ReleaseAndGetAddressOf());
            IFCFAILFAST(HandleROEClosed(hr));

            // The XamlLight will also need to rewalk its subtree to pick up targets for its new WUC light
            m_needsSubtreeRewalk = true;
        }

        m_lightId.Reset();

        CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
    }
}

const xstring_ptr& CXamlLight::GetLightId()
{
    if (m_lightId.IsNull())
    {
        DirectUI::XamlLight* dxamlLight = static_cast<DirectUI::XamlLight*>(GetDXamlPeer());

        wrl::Wrappers::HString idHstring;
        IFCFAILFAST(dxamlLight->GetIdProtected(idHstring.GetAddressOf()));
        IFCFAILFAST(xstring_ptr::CloneRuntimeStringHandle(idHstring.Get(), &m_lightId));
    }

    return m_lightId;
}

bool CXamlLight::IsEnabledInXamlIsland()
{
    //
    // MUX reveal brushes use a solid color fallback when rendered inside a Xaml island. To get correct
    // reveal border behavior, we'll need to light up the brush even when the pointer approaches from
    // outside the Xaml island. The only practical way to do that is with a global light supplied by the
    // shell. Microsoft.UI.Xaml.Controls can access this brush, but MUX can't, so we're going with a solid
    // color fallback for MUX.
    //
    // This in turn means we should prevent lights from targeting MUX reveal brushes inside Xaml islands.
    // Otherwise they'll mess with the color of the brush. If no light is targeting a visual using a solid
    // color brush, then it gets the default ambient light which is what we're after.
    //
    // The quick way to prevent lights from targeting MUX reveal brushes is to not pick up the MUX reveal
    // lights during the render walk. This has the side effect of not allowing the MUX reveal lights to
    // target anything else (e.g. a custom brush, or a specific element), but this is not expected to be
    // a problem because we don't expect apps to depend on its ID.
    //
    const xstring_ptr& lightId = GetLightId();
    return !lightId.Equals(XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml.Media.RevealBorderLight_DarkTheme"))
        && !lightId.Equals(XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml.Media.RevealBorderLight_LightTheme"))
        && !lightId.Equals(XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml.Media.RevealHoverLight"))
        && !lightId.Equals(XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml.Media.XamlAmbientLight"));
}

_Check_return_ HRESULT CXamlLight::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    if (params.fIsLive)
    {
        m_needsSubtreeRewalk = true;

        DirectUI::XamlLight* dxamlLight = static_cast<DirectUI::XamlLight*>(GetDXamlPeer());
        CDependencyObject* parent = GetParentInternal(false);
        DirectUI::UIElement* parentUIE = static_cast<DirectUI::UIElement*>(parent->GetDXamlPeer());
        IFCFAILFAST(dxamlLight->OnConnectedProtected(parentUIE));
    }

    IFCFAILFAST(__super::EnterImpl(pNamescopeOwner, params));

    return S_OK;
}

_Check_return_ HRESULT CXamlLight::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    if (params.fIsLive)
    {
        RemoveAllTargetElements();
        if (HasWUCLight())
        {
            SetCoordinateSpace(nullptr);
        }

        DirectUI::XamlLight* dxamlLight = static_cast<DirectUI::XamlLight*>(GetDXamlPeer());
        CDependencyObject* parent = GetParentInternal(false);
        DirectUI::UIElement* parentUIE = static_cast<DirectUI::UIElement*>(parent->GetDXamlPeer());
        IFCFAILFAST(dxamlLight->OnDisconnectedProtected(parentUIE));
    }

    IFCFAILFAST(__super::LeaveImpl(pNamescopeOwner, params));

    return S_OK;
}

void CXamlLight::AddTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual)
{
    // It's possible the WUC light targets were released after we hit an RO_E_CLOSED. No-op in that case.
    if (m_wucLightTargets != nullptr)
    {
        auto pair = m_targetedVisuals.find(target);
        if (pair != m_targetedVisuals.end())
        {
            pair->second.insert(visual);
        }
        else
        {
            std::unordered_set<WUComp::IVisual*> set;
            set.insert(visual);
            VERIFY_COND(m_targetedVisuals.emplace(target, set), .second);
        }

        HRESULT hr = m_wucLightTargets->Add(visual);
        IFCFAILFAST(HandleROEClosed(hr));
    }
}

void CXamlLight::RemoveTargetElement(_In_ CUIElement* target)
{
    // It's possible the WUC light targets were released after we hit an RO_E_CLOSED. No-op in that case.
    if (m_wucLightTargets != nullptr)
    {
        auto elementPair = m_targetedVisuals.find(target);
        if (elementPair != m_targetedVisuals.end())
        {
            auto& visuals = elementPair->second;

            for (const auto& visual : visuals)
            {
                HRESULT hr = m_wucLightTargets->Remove(visual);
                IFCFAILFAST(HandleROEClosed(hr));
                if (!m_wucLightTargets)
                {
                    break;
                }
            }
        }

        m_targetedVisuals.erase(target);
    }
}

bool CXamlLight::RemoveTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual)
{
    // It's possible the WUC light targets were released after we hit an RO_E_CLOSED. No-op in that case.
    if (m_wucLightTargets != nullptr)
    {
        auto elementPair = m_targetedVisuals.find(target);
        if (elementPair != m_targetedVisuals.end())
        {
            auto& visuals = elementPair->second;

            // It's possible that the visual isn't actually being targeted by this light. When we recycle the visuals inside
            // a content dirty UIElement's render data list, we unregister that visual with all lights that target the UIElement.
            // A border element might have light A that targets its border visual and light B that targets its background, and
            // it will try to remove both visuals from both lights when it recycles the visuals. When we try to remove the
            // background visual from light A or the border visual from light B we will find that they aren't actually targeted.
            // No-op in those cases.
            auto visualPair = visuals.find(visual);
            if (visualPair != visuals.end())
            {
                HRESULT hr = m_wucLightTargets->Remove(visual);
                IFCFAILFAST(HandleROEClosed(hr));

                visuals.erase(visual);
                if (visuals.empty())
                {
                    m_targetedVisuals.erase(elementPair);
                }

                return true;
            }
        }
    }

    return false;
}

void CXamlLight::RemoveAllTargetElements()
{
    // It's possible the WUC light targets were released after we hit an RO_E_CLOSED. No-op in that case.
    if (m_wucLightTargets != nullptr)
    {
        for (const auto& pair : m_targetedVisuals)
        {
            pair.first->UntargetByLight(this);
            for (const auto& visual : pair.second)
            {
                HRESULT hr = m_wucLightTargets->Remove(visual);
                IFCFAILFAST(HandleROEClosed(hr));
                if (hr == RO_E_CLOSED)
                {
                    return;
                }
            }
        }
        m_targetedVisuals.clear();
    }
}

bool CXamlLight::TargetsElement(_In_ CUIElement* element)
{
    auto elementPair = m_targetedVisuals.find(element);
    return elementPair != m_targetedVisuals.end();
}

void CXamlLight::SetCoordinateSpace(_In_opt_ WUComp::IVisual* visual)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<WUComp::ISpotLight> spotLight;
    wrl::ComPtr<WUComp::IPointLight> pointLight;
    wrl::ComPtr<WUComp::IDistantLight> distantLight;

    // It's possible the WUC light was released after we hit an RO_E_CLOSED. No-op in that case.
    if (m_wucLight != nullptr)
    {
        if (SUCCEEDED(m_wucLight.As(&spotLight)))
        {
            hr = spotLight->put_CoordinateSpace(visual);
        }
        else if (SUCCEEDED(m_wucLight.As(&pointLight)))
        {
            hr = pointLight->put_CoordinateSpace(visual);
        }
        else if (SUCCEEDED(m_wucLight.As(&distantLight)))
        {
            hr = distantLight->put_CoordinateSpace(visual);
        }
        IFCFAILFAST(HandleROEClosed(hr));
    }
}
