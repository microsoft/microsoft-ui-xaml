// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RevealBorderLight.h"
#include "IsTargetDPHelper.h"
#include "SpotLightStateHelper.h"
#include "MaterialHelper.h"

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "Microsoft.UI.Composition.Effects_impl.h"
#pragma warning(pop)

#include "RevealBorderLight.properties.cpp"

static constexpr RevealBorderSpotlightStateDesc s_revealBorderSpotlightState
{
    // SpotlightProperty, PropertyName, Value
    { SpotlightProperty::ConstantAttenuation    , L"ConstantAttenuation"    , 1.f },                     // ConstantAttenuation
    { SpotlightProperty::LinearAttenuation      , L"LinearAttenuation"      , 3.f },                     // LinearAttenuation
    { SpotlightProperty::InnerConeAngleInDegrees, L"InnerConeAngleInDegrees", 0.f },                     // InnerConeAngleInDegrees
    { SpotlightProperty::InnerConeColor         , L"InnerConeColor"         ,{ 255, 255, 255, 255 } },   // InnerConeColor
    { SpotlightProperty::OuterConeAngleInDegrees, L"OuterConeAngleInDegrees", 16.94532f },               // OuterConeAngleInDegrees
    { SpotlightProperty::OuterConeColor         , L"OuterConeColor"         ,{ 255, 255, 255, 255 } },   // OuterConeColor
    { SpotlightProperty::Height                 , L"SpotlightHeight"        , 128.f },                   // Height
    { SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , 1.f },                     // OuterAngleScale
};

static constexpr RevealBorderSpotlightStateDesc s_revealBorderSpotlightStateWide
{
    // SpotlightProperty, PropertyName, Value
    { SpotlightProperty::ConstantAttenuation    , L"ConstantAttenuation"    , 1.f },                     // ConstantAttenuation
    { SpotlightProperty::LinearAttenuation      , L"LinearAttenuation"      , 0.5f },                    // LinearAttenuation
    { SpotlightProperty::InnerConeAngleInDegrees, L"InnerConeAngleInDegrees", 0.f },                     // InnerConeAngleInDegrees
    { SpotlightProperty::InnerConeColor         , L"InnerConeColor"         ,{ 255, 255, 255, 255 } },   // InnerConeColor
    { SpotlightProperty::OuterConeAngleInDegrees, L"OuterConeAngleInDegrees", 16.7403f },                // OuterConeAngleInDegrees
    { SpotlightProperty::OuterConeColor         , L"OuterConeColor"         ,{ 255, 255, 255, 255 } },   // OuterConeColor
    { SpotlightProperty::Height                 , L"SpotlightHeight"        , 256.f },                   // Height
    { SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , 1.f },                     // OuterAngleScale
};

static constexpr auto c_PointerOffsetExpression = L"pointer.Position + Vector3(0,0,props.SpotlightHeight)";

winrt::hstring& RevealBorderLight::GetLightThemeIdStatic()
{
    static winrt::hstring s_revealBorderLightLightTheme{ MUXCONTROLSMEDIA_NAMESPACE_STR L".RevealBorderLight_LightTheme" };
    return s_revealBorderLightLightTheme;
}

winrt::hstring& RevealBorderLight::GetDarkThemeIdStatic()
{
    static winrt::hstring s_revealBorderLightDarkTheme{ MUXCONTROLSMEDIA_NAMESPACE_STR L".RevealBorderLight_DarkTheme" };
    return s_revealBorderLightDarkTheme;
}

winrt::hstring RevealBorderLight::GetId()
{
    return m_isLightTheme ? GetLightThemeIdStatic() : GetDarkThemeIdStatic();
}

void RevealBorderLight::OnConnected(winrt::UIElement const& newElement)
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
                    MaterialHelper::LightTemplates<RevealBorderLight>::OnLightTransparencyPolicyChanged(
                        weakThis,
                        sender,
                        dispatcherQueue,
                        false /* onUIThread */);
                }
                });
        }
    }

    // Apply Initial policy state
    MaterialHelper::LightTemplates<RevealBorderLight>::OnLightTransparencyPolicyChanged(
        get_weak(),
        m_materialProperties,
        m_dispatcherQueue,
        true /* onUIThread */);

    m_additionalMaterialPolicyChangedToken = MaterialHelper::AdditionalPolicyChanged([this](auto sender) { OnAdditionalMaterialPolicyChanged(sender); });
#else
    m_materialPolicyChangedToken = MaterialHelper::PolicyChanged([this](auto sender, auto args) { OnMaterialPolicyStatusChanged(sender, args); });
#endif

    // Note this is needed in BUILD_WINDOWS case in case we end up falling back to Local lights
    m_targetElement = winrt::make_weak(newElement);
    if (!m_isDisabledByMaterialPolicy)
    {
        EnsureCompositionResources();
    }
}

void RevealBorderLight::EnsureCompositionResources()
{
#if BUILD_WINDOWS
    if (!m_sharedLight && !m_compositionSpotLight)
    {
        auto compositor = winrt::Window::Current().Compositor();

        winrt::IInspectable sharedLightInsp = nullptr;

        if (m_materialProperties)
        {
            sharedLightInsp =
                m_materialProperties.TryGetLight(
                    m_isWideLight ? winrt::Windows::UI::LightType::RevealBorderWide : winrt::Windows::UI::LightType::RevealBorder,
                    compositor
                );
        }

        // Fallback light is maintained for platforms that do not implement MaterialProperties but still use Reveal.
        // Reveal would be difficult to implement in XamlIslands using local lights in a way that maintained continuity, so Brush fallback is used instead.
        if (!sharedLightInsp)
        {
            auto target = m_targetElement.get();
            winrt::XamlIsland xamlIsland = winrt::XamlIsland::GetIslandFromElement(target);

            if (xamlIsland)
            {
                MaterialHelper::RevealBorderLightUnavailable(true);
            }
            else
            {
                m_fallbackToLocalLight = true;
                EnsureLocalLight();
            }
        }
        else
        {
            m_sharedLight = sharedLightInsp.try_as<winrt::SharedLight>();
            CompositionLight(m_sharedLight);
        }
    }
#else
    EnsureLocalLight();
#endif
}

void RevealBorderLight::EnsureLocalLight()
{
    if (!m_compositionSpotLight)
    {
        auto compositor = winrt::Window::Current().Compositor();
        m_compositionSpotLight = compositor.CreateSpotLight();
        CompositionLight(m_compositionSpotLight);

        m_colorsProxy = CreateSpotLightColorsProxy(m_compositionSpotLight);
        m_offsetProps = compositor.CreatePropertySet();

        m_offsetProps.InsertScalar(L"SpotlightHeight", m_isWideLight ? s_revealBorderSpotlightStateWide.Height.Value : s_revealBorderSpotlightState.Height.Value);
        SetSpotLightStateImmediate(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_isWideLight ? s_revealBorderSpotlightStateWide : s_revealBorderSpotlightState);

        HookupWindowPointerHandlers();

        // Create an expression animation that combines the PointerPositionPropertySet pointer position and the z-depth of the light
        if (auto targetElement = m_targetElement.get())
        {
            m_offsetAnimation = compositor.CreateExpressionAnimation(c_PointerOffsetExpression);
            m_offsetAnimation.SetReferenceParameter(L"props", m_offsetProps);

            m_pointer = winrt::ElementCompositionPreview::GetPointerPositionPropertySet(targetElement);
            m_offsetAnimation.SetReferenceParameter(L"pointer", m_pointer);

            m_compositionSpotLight.StartAnimation(L"Offset", m_offsetAnimation);
        }
    }
}

void RevealBorderLight::ReleaseCompositionResources()
{
#if BUILD_WINDOWS
    m_sharedLight = nullptr;
    if (m_fallbackToLocalLight)
    {
        ReleaseLocalLight();
    }
#else 
    ReleaseLocalLight();
#endif
    CompositionLight(nullptr);
}

void RevealBorderLight::ReleaseLocalLight()
{
    m_colorsProxy = nullptr;
    m_shouldLightBeOn = false;
    UnhookWindowPointerHandlers();
    if (m_compositionSpotLight)
    {
        m_compositionSpotLight.StopAnimation(L"Offset");
    }
    if (m_pointer)
    {
        m_pointer.Close();
        m_pointer = nullptr;
    }
    m_offsetAnimation = nullptr;
    m_offsetProps = nullptr;
    m_compositionSpotLight = nullptr;
}


void RevealBorderLight::OnDisconnected(winrt::UIElement const& /*oldElement*/)
{
    ReleaseCompositionResources();
    m_targetElement = nullptr;

#if BUILD_WINDOWS
    MaterialHelper::AdditionalPolicyChanged(m_additionalMaterialPolicyChangedToken);
    m_additionalMaterialPolicyChangedToken.value = 0;
#else
    MaterialHelper::PolicyChanged(m_materialPolicyChangedToken);
    m_materialPolicyChangedToken.value = 0;
#endif
}

#if BUILD_WINDOWS
winrt::SharedLight RevealBorderLight::GetSharedLight()
{
    return m_sharedLight;
}

bool RevealBorderLight::GetFallbackToLocalLight()
{
    return m_fallbackToLocalLight;
}

void RevealBorderLight::OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender)
{
    MaterialHelper::LightTemplates<RevealBorderLight>::OnLightTransparencyPolicyChanged(
        get_weak(),
        m_materialProperties,
        m_dispatcherQueue,
        true /* onUIThread */);
}
#else
void RevealBorderLight::OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy)
{
    MaterialHelper::LightPolicyChangedHelper<RevealBorderLight>(this, isDisabledByMaterialPolicy);
}
#endif

bool RevealBorderLight::GetShouldLightBeOn()
{
    return m_shouldLightBeOn;
}

// Flip the light switch and toggle the expression animations that correctly size and position it (i.e. stop the animations when light is off).
void RevealBorderLight::SwitchLight(bool turnOn)
{
    if ((m_shouldLightBeOn == turnOn) ||
        m_isDisabledByMaterialPolicy ||
        !m_colorsProxy)
    {
        return;
    }

    m_shouldLightBeOn = turnOn;

    const auto animateSpotLight = [this, turnOn]()
    {
        auto animation = m_colorsProxy.Compositor().CreateScalarKeyFrameAnimation();
        animation.Duration(250ms);
        animation.InsertKeyFrame(1, turnOn ? 1.f : 0.f);
        m_colorsProxy.StartAnimation(L"LightIntensity", animation);
    };

    // Also use the more performanant IsEnabled property if available (RS4+).
    auto lightWithEnabledProperty = m_compositionSpotLight.try_as<winrt::ICompositionLight3>();

    if (turnOn)
    {
        if (lightWithEnabledProperty)
        {
            // If TurnOn animation was started while a TurnOff animation is running, the TurnOff Completed
            // event fires after TurnOn starts, causing IsEnabled to end up at False. Set flag to prevent this scenario here.
            m_setLightDisabledAfterTurnOffAnimation = false;
            lightWithEnabledProperty.IsEnabled(true);
        }
        animateSpotLight();
    }
    else
    {
        winrt::CompositionScopedBatch scopedBatch = winrt::Window::Current().Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        animateSpotLight();
        scopedBatch.End();

        if (lightWithEnabledProperty)
        {
            m_setLightDisabledAfterTurnOffAnimation = true;
            auto strongThis = get_strong();

            scopedBatch.Completed(
                [strongThis, lightWithEnabledProperty](auto sender, auto args)
                {
                    if (strongThis->m_setLightDisabledAfterTurnOffAnimation)
                    {
                        lightWithEnabledProperty.IsEnabled(false);
                        strongThis->m_setLightDisabledAfterTurnOffAnimation = false;
                    }
                });
        }
    }
}

void RevealBorderLight::HookupWindowPointerHandlers()
{
    auto coreWindow = winrt::Window::Current().CoreWindow();
    WINRT_ASSERT(coreWindow);       // CoreWindow presence required for Reveal experience
    m_coreWindow = coreWindow;

    if (m_coreWindow)
    {
        auto strongThis = get_strong();

        // Make sure RevealBorderLight is alive and the token is valid before executing each handler's body.
        // Otherwise, it's possible for the handler to get invoked after we've unsubscribed from the event and the object has been destroyed.
        // See comments in UnhookWindowPointerHandlers() for more details.
        m_PointerEnteredToken = m_coreWindow.PointerEntered(
            [strongThis](const winrt::CoreWindow&, const winrt::PointerEventArgs&)
            {
                if (strongThis->m_PointerEnteredToken.value != 0)
                {
                    strongThis->SwitchLight(true);
                }
            });

        m_PointerExitedToken = m_coreWindow.PointerExited(
            [strongThis](const winrt::CoreWindow&, const winrt::PointerEventArgs&)
            {
                if (strongThis->m_PointerExitedToken.value != 0)
                {
                    strongThis->SwitchLight(false);
                }
            });

        m_PointerMovedToken = m_coreWindow.PointerMoved(
            [strongThis](const winrt::CoreWindow&, const winrt::PointerEventArgs& args)
            {
                if (strongThis->m_PointerMovedToken.value != 0)
                {
                    // On PointerMoved, make sure that the border light is on, except in the following cases:
                    // 
                    // 1. This is a Touch or Pen PointerMoved. This should not be necessary as we already listen for events that mark when a touch interaction starts 
                    //    (PointerEntered/Pressed) and ends (PointerExited/Released/CaptureLost. Also, in the case of dragging items in a ListView with CanDragItems = true, 
                    //    we get a touch PointerMoved after completing the drag, which keeps the light on at the touch point (where DManip first took over).
                    //
                    // 2. This is the first PointerMoved after a CaptureLost event we got due to DManip taking over touch or pen input (eg pan in ScrollViewer).
                    //    In this case, after the touch/pen interaction is complete, we occasionally get a PointerMoved on Mouse for an unknown reason.
                    //    Ignore this event, as it will otherwise keep the light on at the touch point (where DManip first took over).
                    if (args.CurrentPoint().PointerDevice().PointerDeviceType() == winrt::Devices::Input::PointerDeviceType::Mouse &&
                        !strongThis->m_gotPointerCaptureLostDueToDManip)
                    {
                        strongThis->SwitchLight(true);
                    }

                    strongThis->m_gotPointerCaptureLostDueToDManip = false;
                }
            });

        m_PointerPressedToken = m_coreWindow.PointerPressed(
            [strongThis](const winrt::CoreWindow&, const winrt::PointerEventArgs&)
            {
                if (strongThis->m_PointerPressedToken.value != 0)
                {
                    strongThis->SwitchLight(true);
                }
            });

        m_PointerReleasedToken = m_coreWindow.PointerReleased(
            [strongThis](const winrt::CoreWindow&, const winrt::PointerEventArgs& args)
            {
                if (strongThis->m_PointerReleasedToken.value != 0)
                {
                    if (args.CurrentPoint().PointerDevice().PointerDeviceType() == winrt::Devices::Input::PointerDeviceType::Touch)
                    {
                        strongThis->SwitchLight(false);
                    }
                }
            });

        m_PointerCaptureLostToken = m_coreWindow.PointerCaptureLost(
            [strongThis](const winrt::CoreWindow&, const winrt::PointerEventArgs& args)
            {
                if (strongThis->m_PointerCaptureLostToken.value != 0)
                {
                    strongThis->SwitchLight(false);

                    if (args.CurrentPoint().PointerDevice().PointerDeviceType() == winrt::Devices::Input::PointerDeviceType::Pen ||
                        args.CurrentPoint().PointerDevice().PointerDeviceType() == winrt::Devices::Input::PointerDeviceType::Touch)
                    {
                        strongThis->m_gotPointerCaptureLostDueToDManip = true;
                    }
                }
            });
    }
}

void RevealBorderLight::UnhookWindowPointerHandlers()
{
    if (m_coreWindow)
    {
        // It's possible for UnhookWindowPointerHandlers() to be called while handling another CoreWindow.PointerExited.
        // In this case, CoreWindow's list of handlers is locked and the scheduled PointerExited will fire even after beig unsubscribed.
        // Clear the token after unregistering so any suuch callbacks can be ignored.
        if (m_PointerEnteredToken.value != 0)
        {
            m_coreWindow.PointerEntered(m_PointerEnteredToken);
            m_PointerEnteredToken.value = 0;
        }

        if (m_PointerExitedToken.value != 0)
        {
            m_coreWindow.PointerExited(m_PointerExitedToken);
            m_PointerExitedToken.value = 0;
        }

        if (m_PointerMovedToken.value != 0)
        {
            m_coreWindow.PointerMoved(m_PointerMovedToken);
            m_PointerMovedToken.value = 0;
        }

        if (m_PointerPressedToken.value != 0)
        {
            m_coreWindow.PointerPressed(m_PointerPressedToken);
            m_PointerPressedToken.value = 0;
        }

        if (m_PointerReleasedToken.value != 0)
        {
            m_coreWindow.PointerReleased(m_PointerReleasedToken);
            m_PointerReleasedToken.value = 0;
        }

        if (m_PointerCaptureLostToken.value != 0)
        {
            m_coreWindow.PointerCaptureLost(m_PointerCaptureLostToken);
            m_PointerCaptureLostToken.value = 0;
        }

        m_coreWindow = nullptr;
    }
}
