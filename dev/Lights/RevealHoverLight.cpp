// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RevealHoverLight.h"
#include "IsTargetDPHelper.h"
#include "SpotLightStateHelper.h"

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "Microsoft.UI.Composition.Effects_impl.h"
#pragma warning(pop)

#include "RevealHoverLight.properties.cpp"

static constexpr EasingInfo c_LinearEasing = EasingInfo{ EasingTypes::Linear,{ 0.f, 0.f },{ 0.f, 0.f } };
static constexpr EasingInfo c_CubicEasing1 = EasingInfo{ EasingTypes::CubicBezier,{ 0.5f, 0.f } ,{ 0.6f, 1.f } };
static constexpr EasingInfo c_CubicEasingRelease = EasingInfo{ EasingTypes::CubicBezier,{ 0.33f, 0.f },{ 0.67f, 1.f } };
static constexpr winrt::TimeSpan c_HalfDuration = 166ms;

// Due to "Bug 14047529: Flash when moving mouse over NavigationView items due to reveal backplate"
// we are removing the proposed AnimToOff/AnimToHover 166ms fade animations until Comp delivers brush cross-fade capability.
// Note that the fade animations are implemented with InnerConeColor/OuterConeColor on RS2 and InnerConeIntensity/OuterConeIntensity on RS3+.
// For RS2, removing the animations has the side effect that hover light to sometimes not show up on tap; this is due to issues in the PointerSourceWrapper's
// simulated touch mode in the RS2 OS release. To work around, we use a 1ms color animation. This can still lead to a flash, but a very brief one.
// For RS3+, we directly switch light on/off, which completely resolves the flash.
static constexpr std::array<RevealHoverSpotlightStateDesc, RevealHoverSpotlightState_StateCount> sc_revealHoverSpotlightStates = { {
        // AnimToOff
        // Flags, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeColor         , L"InnerConeColor"         , 1ms, { 255, 0, 0, 0 } },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeColor         , L"OuterConeColor"         , 1ms, { 255, 0, 0, 0 } },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity"     , c_HalfDuration, 0.f },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity"     , c_HalfDuration, 0.f },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , c_HalfDuration, 1.f },
        },

        // AnimToHover
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeColor         , L"InnerConeColor"         , 1ms, { 255, 255, 255, 255 } },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeColor         , L"OuterConeColor"         , 1ms, { 255, 255, 255, 255 } },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity"     , c_HalfDuration, 1.f },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity"     , c_HalfDuration, 1.f },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , c_HalfDuration, 1.f },
        },

        // Pressing
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::InnerConeColor         , L"InnerConeColor" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterConeColor         , L"OuterConeColor" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterAngleScale        , L"OuterAngleScale" },
        },

        // FastRelease
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::InnerConeColor         , L"InnerConeColor" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterConeColor         , L"OuterConeColor" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterAngleScale        , L"OuterAngleScale" },
        },

        // SlowRelease
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::InnerConeColor         , L"InnerConeColor" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterConeColor         , L"OuterConeColor" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity" },
            { RevealHoverSpotlightFlags::Ignore,  SpotlightProperty::OuterAngleScale        , L"OuterAngleScale" },
        },

    } };

static constexpr std::array<RevealHoverSpotlightStateDesc, RevealHoverSpotlightState_StateCount> sc_pressSpotLightStates = { {
        // AnimToOff
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeColor         , L"InnerConeColor"         , 1ms, { 255, 0, 0, 0 },      c_CubicEasing1 },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeColor         , L"OuterConeColor"         , 1ms, { 255, 0, 0, 0 },      c_CubicEasing1 },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity"     , c_HalfDuration, 0.f,        c_CubicEasing1 },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity"     , c_HalfDuration, 0.f,        c_CubicEasing1 },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , c_HalfDuration, 1.f,        c_CubicEasing1 },
        },

        // AnimToHover
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeColor         , L"InnerConeColor"         , 1ms, { 255, 0, 0, 0 },       c_CubicEasing1 },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeColor         , L"OuterConeColor"         , 1ms, { 255, 0, 0, 0 },       c_CubicEasing1 },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity"     , c_HalfDuration, 0.f,         c_CubicEasing1 },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity"     , c_HalfDuration, 0.f,         c_CubicEasing1 },
            { RevealHoverSpotlightFlags::None,    SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , c_HalfDuration, 1.f,         c_CubicEasing1 },
        },

        // Pressing
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeColor         , L"InnerConeColor"         , 66ms,{ 255, 255, 255, 255 },            c_LinearEasing },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeColor         , L"OuterConeColor"         , 66ms,{ 255, 255, 255, 255 },            c_LinearEasing },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity"     , 66ms, 1.f,                              c_LinearEasing },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity"     , 66ms, 1.f,                              c_CubicEasing1 },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , 66ms, 1.f,                              c_LinearEasing },
        },

        // FastRelease
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeColor         , L"InnerConeColor"         , 200ms,{ 255, 0, 0, 0 },                c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeColor         , L"OuterConeColor"         , 266ms,{ 255, 0, 0, 0 },                c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity"     , 200ms, 0.f,                             c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity"     , 266ms, 0.f,                             c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , 266ms, 4.44f,                           c_CubicEasingRelease },
        },

        // SlowRelease
        // Ignore, Animate, SpotlightProperty, PropertyName, Duration, Value, EasingType, EasingControlPoint1, EasingControlPoint2
        {
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeColor         , L"InnerConeColor"         , 1000ms,{ 255, 0, 0, 0 },               c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeColor         , L"OuterConeColor"         , 1500ms,{ 255, 0, 0, 0 },               c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::InnerConeIntensity     , L"InnerConeIntensity"     , 1000ms, 0.f,                            c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterConeIntensity     , L"OuterConeIntensity"     , 1500ms, 0.f,                            c_CubicEasingRelease },
            { RevealHoverSpotlightFlags::Animate, SpotlightProperty::OuterAngleScale        , L"OuterAngleScale"        , 1500ms, 4.44f,                          c_CubicEasingRelease },
        },
    } };

#if DBG
// NOTE: These arrays exist because in debug they need to be mutable so that the test API can change values dynamically.
// We default them to the const data blobs above but then they can be modified. Below the code chooses the mutable one
// in debug and the const one in retail.
std::array<RevealHoverSpotlightStateDesc, RevealHoverSpotlightState_StateCount> RevealHoverLight::s_revealHoverSpotlightStates = sc_revealHoverSpotlightStates;
std::array<RevealHoverSpotlightStateDesc, RevealHoverSpotlightState_StateCount> RevealHoverLight::s_pressSpotLightStates = sc_pressSpotLightStates;

float RevealHoverLight::s_lightMinSize{ 16.0f };
float RevealHoverLight::s_lightMaxSize{ 512.0f };
float RevealHoverLight::s_sizeAdjustment{ 12.0f };
float RevealHoverLight::s_pressOuterSize{ 47.25f };
float RevealHoverLight::s_innerConeAngleInDegrees{ 0.f };
float RevealHoverLight::s_outerConeAngleInDegrees{ 0.f };
float RevealHoverLight::s_spotlightHeight{ 256.f };
float RevealHoverLight::s_constantAttenuation{ 2.f };
float RevealHoverLight::s_linearAttenuation{ 0.5f };
static constexpr auto c_PointerOffsetExpression = L"pointer.Position + Vector3(0, 0, props.SpotlightHeight)";
static constexpr auto c_CenteredOffsetExpression = L"Vector3((visual.Size.X / 2), (visual.Size.Y / 2), props.SpotlightHeight)";
static constexpr auto c_OuterAngleExpression = L"ATan((Clamp(Max(visual.Size.X, visual.Size.Y) + props.SizeAdjustment, props.MinSize, props.MaxSize) * props.OuterAngleScale) / props.SpotlightHeight)";
static constexpr auto c_PressOuterAngleExpression = L"ATan(((props.PressOuterSize) * props.OuterAngleScale) / props.SpotlightHeight)";
#else
const float RevealHoverLight::s_constantAttenuation{ 2.f };
const float RevealHoverLight::s_linearAttenuation{ 0.5f };
static constexpr auto c_PointerOffsetExpression = L"pointer.Position + Vector3(0, 0, 256)";
static constexpr auto c_CenteredOffsetExpression = L"Vector3((visual.Size.X / 2), (visual.Size.Y / 2), 256)";
static constexpr auto c_OuterAngleExpression = L"ATan((Clamp(Max(visual.Size.X, visual.Size.Y) + 12, 16, 512) * props.OuterAngleScale) / 256)";
static constexpr auto c_PressOuterAngleExpression = L"ATan(((47.25) * props.OuterAngleScale) / 256)";
#endif


winrt::hstring& RevealHoverLight::GetLightIdStatic()
{
    static winrt::hstring s_RevealHoverLightId{ MUXCONTROLSMEDIA_NAMESPACE_STR L".RevealHoverLight" };
    return s_RevealHoverLightId;
}

winrt::hstring RevealHoverLight::GetId()
{
    return GetLightIdStatic();
}

void RevealHoverLight::OnConnected(winrt::UIElement const& newElement)
{
    m_targetElement = winrt::make_weak(newElement);

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
                    MaterialHelper::LightTemplates<RevealHoverLight>::OnLightTransparencyPolicyChanged(
                        weakThis,
                        sender,
                        dispatcherQueue,
                        false /* onUIThread */);
                }
                });
        }
    }

    // Apply Initial policy state
    MaterialHelper::LightTemplates<RevealHoverLight>::OnLightTransparencyPolicyChanged(
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

void RevealHoverLight::OnDisconnected(winrt::UIElement const& /*oldElement*/)
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

void RevealHoverLight::EnsureCompositionResources()
{
    if (!m_compositionSpotLight)
    {
        if (auto element = m_targetElement.get())
        {
            auto compositor = winrt::Window::Current().Compositor();

            m_compositionSpotLight = compositor.CreateSpotLight();
            CompositionLight(m_compositionSpotLight);

            // Set non-default constant values
            m_compositionSpotLight.ConstantAttenuation(s_constantAttenuation);
            m_compositionSpotLight.LinearAttenuation(s_linearAttenuation);

            // Not initializing these prevents spotlight from rendering
            m_compositionSpotLight.InnerConeAngleInDegrees(0);
            m_compositionSpotLight.OuterConeAngleInDegrees(0);

            // Set non-animatable initial state. In DBG mode, it can be adjusted via the PropertySet through RevealTestApi
            m_offsetProps = compositor.CreatePropertySet();
#if DBG
            m_offsetProps.InsertScalar(L"MinSize", s_lightMinSize);
            m_offsetProps.InsertScalar(L"MaxSize", s_lightMaxSize);
            m_offsetProps.InsertScalar(L"SizeAdjustment", s_sizeAdjustment);
            m_offsetProps.InsertScalar(L"PressOuterSize", s_pressOuterSize);
            m_offsetProps.InsertScalar(L"SpotlightHeight", s_spotlightHeight);
#endif

            m_offsetAnimation = compositor.CreateExpressionAnimation(c_CenteredOffsetExpression);
            m_offsetAnimation.SetReferenceParameter(L"props", m_offsetProps);

            m_outerAngleAnimation = compositor.CreateExpressionAnimation(m_isPressLight ? c_PressOuterAngleExpression : c_OuterAngleExpression);
            m_outerAngleAnimation.SetReferenceParameter(L"props", m_offsetProps);

            m_colorsProxy = CreateSpotLightColorsProxy(m_compositionSpotLight);

            // hook into PointerPressed event so we know if pressed is invoked by mouse/touch versus keyboard
            m_elementPointerPressedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &RevealHoverLight::OnPointerPressed });
            element.AddHandler(winrt::UIElement::PointerPressedEvent(), m_elementPointerPressedEventHandler, true);

            // Set animatable initial state
#if DBG
            m_spotLightStates = m_isPressLight ? s_pressSpotLightStates.data() : s_revealHoverSpotlightStates.data();
#else
            m_spotLightStates = m_isPressLight ? sc_pressSpotLightStates.data() : sc_revealHoverSpotlightStates.data();
#endif
            SetSpotLightStateImmediate(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_AnimToOff]);

            m_pointer = winrt::ElementCompositionPreview::GetPointerPositionPropertySet(element);
            const auto elementVisual = winrt::ElementCompositionPreview::GetElementVisual(element);

            m_offsetAnimation.SetReferenceParameter(L"pointer", m_pointer);
            m_offsetAnimation.SetReferenceParameter(L"visual", elementVisual);
            m_outerAngleAnimation.SetReferenceParameter(L"visual", elementVisual);

            if (m_shouldLightBeOn)
            {
                SwitchLight(true);
            }
        }
    }
}

void RevealHoverLight::ReleaseCompositionResources()
{
    CancelCurrentPressStateContinuation();

    if (m_targetElement)
    {
        auto element = m_targetElement.get();

        if (m_elementPointerPressedEventHandler)
        {
            // clean up PointerPressed event
            element.RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_elementPointerPressedEventHandler);
        }
    }
    m_elementPointerPressedEventHandler = nullptr;

    DisableHoverAnimation();
    m_offsetAnimation = nullptr;
    m_outerAngleAnimation = nullptr;
    m_compositionSpotLight = nullptr;

    if (m_pointer)
    {
        m_pointer.Close();
        m_pointer = nullptr;
    }

    m_colorsProxy = nullptr;
    m_offsetAnimation = nullptr;
    m_offsetProps = nullptr;

    CompositionLight(nullptr);
}

#if BUILD_WINDOWS
void RevealHoverLight::OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender)
{
    MaterialHelper::LightTemplates<RevealHoverLight>::OnLightTransparencyPolicyChanged(
        get_weak(),
        m_materialProperties,
        m_dispatcherQueue,
        true /* onUIThread */);
}
#else
void RevealHoverLight::OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy)
{
    MaterialHelper::LightPolicyChangedHelper<RevealHoverLight>(this, isDisabledByMaterialPolicy);

}
#endif

void RevealHoverLight::GoToState(_In_ winrt::RevealBrushState newState)
{
    switch (newState)
    {
    case winrt::RevealBrushState::Normal:
        GotoLightState(LightEvents::GotoNormal);
        break;
    case winrt::RevealBrushState::PointerOver:
        GotoLightState(LightEvents::GotoPointerOver);
        break;
    case winrt::RevealBrushState::Pressed:
        GotoLightState(LightEvents::GotoPressed);
        break;
    }
}

// There are 5 possible states for the hover light: { Off, AnimToHover, AnimToOff, Hover, Pressing }
// There are 4 possible events which can result in a state change: { GotoNormal, GotoPointerOver, GotoPressed, AnimationComplete }. 
//    >> The first three  correspond to VSM state transitionswhile AnimationComplete signifies that an animated state transition is finished. 
//
// Of the 20 possible transitions, 19 (?) either do nothing or switch to another state. There is a little bit of logic nescessary 
// when in the state Releasing and the animation completes (what happens depends on whether the pointer is pressed and / or hovering) 
void RevealHoverLight::GotoLightState(LightEvents e)
{
    // Update pointer flags before transitioning to the next state
    switch (e)
    {
    case LightEvents::GotoNormal:
    {
        m_centerLight = true;
        m_isPointerOver = false;
        m_isPressed = false;
    }
    break;

    case LightEvents::GotoPointerOver:
    {
        m_centerLight = false;
        m_isPointerOver = true;
        m_isPressed = false;
    }
    break;

    case LightEvents::GotoPressed:
    {
        m_isPressed = true;
    }
    break;
    case LightEvents::AnimationComplete:
        break;
    }

    const auto initState = m_currentLightState;
    switch (initState)
    {
    case LightStates::Off:
        switch (e)
        {
        case LightEvents::GotoNormal:
            break;
        case LightEvents::GotoPointerOver:
            GotoLightStateHelper(LightStates::AnimToHover);
            break;
        case LightEvents::GotoPressed:
            GotoLightStateHelper(LightStates::Pressing);
            break;
        case LightEvents::AnimationComplete:
            break;
        }
        break;

    case LightStates::AnimToHover:
        switch (e)
        {
        case LightEvents::GotoNormal:
            GotoLightStateHelper(LightStates::AnimToOff);
            break;
        case LightEvents::GotoPointerOver:
            break;
        case LightEvents::GotoPressed:
            GotoLightStateHelper(LightStates::Pressing);
            break;
        case LightEvents::AnimationComplete:
            GotoLightStateHelper(LightStates::Hover);
            break;
        }
        break;

    case LightStates::AnimToOff:
        switch (e)
        {
        case LightEvents::GotoNormal:
            break;
        case LightEvents::GotoPointerOver:
            GotoLightStateHelper(LightStates::AnimToHover);
            break;
        case LightEvents::GotoPressed:
            GotoLightStateHelper(LightStates::Pressing);
            break;
        case LightEvents::AnimationComplete:
            GotoLightStateHelper(LightStates::Off);
            break;
        }
        break;

    case LightStates::Hover:
        switch (e)
        {
        case LightEvents::GotoNormal:
            GotoLightStateHelper(LightStates::AnimToOff);
            break;
        case LightEvents::GotoPointerOver:
            break;
        case LightEvents::GotoPressed:
            GotoLightStateHelper(LightStates::Pressing);
            break;
        case LightEvents::AnimationComplete:
            break;
        }
        break;

    case LightStates::Pressing:
    {
        // Once the press is done, we always reset to pointer-based offset
        if (m_compositionSpotLight)
        {
            m_offsetAnimation.Expression(c_PointerOffsetExpression);
            m_compositionSpotLight.StartAnimation(L"Offset", m_offsetAnimation);
        }

        switch (e)
        {
        case LightEvents::GotoNormal:
            GotoLightStateHelper(LightStates::AnimToOff);
            break;
        case LightEvents::GotoPointerOver:
            break;
        case LightEvents::GotoPressed:
        {
            // This indicates a press, then release then press again while the first press animation is still going
            GotoLightStateHelper(LightStates::AnimToHover, true);
            GotoLightStateHelper(LightStates::Pressing);
        }
        break;
        case LightEvents::AnimationComplete:
        {
            if (m_isPressed)
            {
                GotoLightStateHelper(LightStates::SlowRelease);
            }
            else
            {
                GotoLightStateHelper(LightStates::FastRelease);
            }
        }
        break;
        }
    }
    break;

    case LightStates::FastRelease:
        switch (e)
        {
        case LightEvents::GotoNormal:
            GotoLightStateHelper(LightStates::AnimToOff);
            break;
        case LightEvents::GotoPointerOver:
            break;
        case LightEvents::GotoPressed:
            // This indicates a press, then release then press again while the first press animation is still going
            GotoLightStateHelper(LightStates::AnimToHover, true);
            GotoLightStateHelper(LightStates::Pressing);
            break;
        case LightEvents::AnimationComplete:
            if (!m_isPressed && m_isPointerOver)
            {
                GotoLightStateHelper(LightStates::AnimToHover, true);
            }
            break;
        }
        break;

    case LightStates::SlowRelease:
        switch (e)
        {
        case LightEvents::GotoNormal:
            GotoLightStateHelper(LightStates::AnimToOff);
            break;
        case LightEvents::GotoPointerOver:
            // This indicates a semi-long press, where button is released before it's done. Complete it with a FastRelease animation.
            GotoLightStateHelper(LightStates::FastRelease);
            break;
        case LightEvents::GotoPressed:
            // This indicates a press, then release then press again while the first press animation is still going
            GotoLightStateHelper(LightStates::AnimToHover, true);
            GotoLightStateHelper(LightStates::Pressing);
            break;
        case LightEvents::AnimationComplete:
            if (!m_isPressed && m_isPointerOver)
            {
                GotoLightStateHelper(LightStates::AnimToHover, true);
            }
            break;
        }
        break;
    }
}

void RevealHoverLight::GotoLightStateHelper(LightStates target, bool skipAnimation)
{
    CancelCurrentPressStateContinuation();

    m_currentLightState = target;

    switch (target)
    {
    case LightStates::Off:
    {
        SwitchLight(false);
    }
    break;

    case LightStates::AnimToOff:
    {
        SwitchLight(true);

        if (skipAnimation)
        {
            SetSpotLightStateImmediate(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_AnimToOff]);
            GotoLightState(LightEvents::AnimationComplete);
        }
        else
        {
            PlaySpotLightStateAnimation(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_AnimToOff], &m_cancelCurrentPressStateContinuation, [=]
                {
                    GotoLightState(LightEvents::AnimationComplete);
                });
        }
    }
    break;

    case LightStates::AnimToHover:
    {
        if (m_compositionSpotLight)
        {
            m_offsetAnimation.Expression(c_PointerOffsetExpression);
            m_compositionSpotLight.StartAnimation(L"Offset", m_offsetAnimation);
        }

        SwitchLight(true);

        if (skipAnimation)
        {
            SetSpotLightStateImmediate(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_AnimToHover]);
            GotoLightState(LightEvents::AnimationComplete);
        }
        else
        {
            PlaySpotLightStateAnimation(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_AnimToHover], &m_cancelCurrentPressStateContinuation, [=]
                {
                    GotoLightState(LightEvents::AnimationComplete);
                });
        }
    }
    break;

    case LightStates::Hover:
    {
        SwitchLight(true);
    }
    break;

    case LightStates::Pressing:
    {
        if (m_compositionSpotLight)
        {
            auto targetControl = m_targetElement.get().try_as<winrt::Control>();

            // m_targetElement can be Grid or other kind of element which is not a control.
            // In this case, we only check m_centerLight, and set focusState directly to keyboard.
            // This is not a perfect solution, The light may appear in the wrong place if apps 
            // apply RevealBrush.State to elements really deep in the template:
            // m_centerLight alone is not correct if reveal light is added on Pressed(as usually happens for touch), 
            // in that case the light misses the PointerPressed event since it's not yet hooked up at that time, 
            // and we will still do keyboard placement.

            // To avoid these types of cases, we should suggest customer that RevealState is always applied to immediate child of a Control
            // Like below, we add UserControl:
            //  <Grid><UserControl><Border Name="myBorderElement"></Border></UserControl></Grid>
            //  then RevealBrush::SetState(myBorderElement, RevealBrushState::Pressed);
            auto const focusState = targetControl ? targetControl.FocusState() : winrt::FocusState::Keyboard;

            // Only center the hover light if the element has KB focus and to the best of our knowledge 
            // it is the keyboard and not pointer that's responsible for current press.
            if (focusState == winrt::FocusState::Keyboard && m_centerLight)
            {
                m_offsetAnimation.Expression(c_CenteredOffsetExpression);
                m_compositionSpotLight.StartAnimation(L"Offset", m_offsetAnimation);
            }
            else
            {
                m_offsetAnimation.Expression(c_PointerOffsetExpression);
                m_compositionSpotLight.StartAnimation(L"Offset", m_offsetAnimation);
            }
        }
        SwitchLight(true);

        if (skipAnimation)
        {
            SetSpotLightStateImmediate(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_Pressing]);
            GotoLightState(LightEvents::AnimationComplete);
        }
        else
        {
            PlaySpotLightStateAnimation(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_Pressing], &m_cancelCurrentPressStateContinuation, [=]
                {
                    GotoLightState(LightEvents::AnimationComplete);
                });
        }
    }
    break;

    case LightStates::FastRelease:
    {
        SwitchLight(true);

        if (skipAnimation)
        {
            SetSpotLightStateImmediate(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_FastRelease]);
            GotoLightState(LightEvents::AnimationComplete);
        }
        else
        {
            PlaySpotLightStateAnimation(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_FastRelease], &m_cancelCurrentPressStateContinuation, [=]
                {
                    GotoLightState(LightEvents::AnimationComplete);
                });
        }
    }
    break;

    case LightStates::SlowRelease:
    {
        SwitchLight(true);

        if (skipAnimation)
        {
            SetSpotLightStateImmediate(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_SlowRelease]);
            GotoLightState(LightEvents::AnimationComplete);
        }
        else
        {
            PlaySpotLightStateAnimation(m_compositionSpotLight, m_colorsProxy, m_offsetProps, m_spotLightStates[RevealHoverSpotlightState_SlowRelease], &m_cancelCurrentPressStateContinuation, [=]
                {
                    GotoLightState(LightEvents::AnimationComplete);
                });
        }
    }
    break;
    }
}

void RevealHoverLight::CancelCurrentPressStateContinuation()
{
    if (m_cancelCurrentPressStateContinuation)
    {
        m_cancelCurrentPressStateContinuation();
        m_cancelCurrentPressStateContinuation = nullptr;
    }
}

void  RevealHoverLight::OnPointerPressed(const winrt::IInspectable& /*sender*/, const winrt::PointerRoutedEventArgs& /*args*/)
{
    m_centerLight = false;

    // Sometimes we get pointer pressed after the VSM has already gone to the pressed state. Handle that case by
    // updating the expression if we get these out of sequence.
    if (m_isPressed && m_compositionSpotLight)
    {
        m_offsetAnimation.Expression(c_PointerOffsetExpression);
        m_compositionSpotLight.StartAnimation(L"Offset", m_offsetAnimation);
    }
}

// Flip the light switch and toggle the expression animations that correctly size and position it (i.e. stop the animations when light is off).
void RevealHoverLight::SwitchLight(bool turnOn)
{
    if (m_compositionSpotLight && (!m_cancelCurrentPressStateContinuation || turnOn))
    {
        m_shouldLightBeOn = turnOn;
        m_shouldLightBeOn ? EnableHoverAnimation() : DisableHoverAnimation();

        m_colorsProxy.InsertScalar(L"LightIntensity", turnOn ? 1.f : 0.f);

        // Also use the more performanant IsEnabled property if it is available (RS4+).
        if (auto lightWithEnabledProperty = m_compositionSpotLight.try_as<winrt::ICompositionLight3>())
        {
            lightWithEnabledProperty.IsEnabled(turnOn);
        }
    }
}

void RevealHoverLight::EnableHoverAnimation()
{
    if (m_compositionSpotLight && !m_isHoverAnimationActive)
    {
        m_isHoverAnimationActive = true;
        m_compositionSpotLight.StartAnimation(L"Offset", m_offsetAnimation);
        m_compositionSpotLight.StartAnimation(L"OuterConeAngle", m_outerAngleAnimation);
    }
}

void RevealHoverLight::DisableHoverAnimation()
{
    if (m_compositionSpotLight && m_isHoverAnimationActive)
    {
        m_isHoverAnimationActive = false;
        m_compositionSpotLight.StopAnimation(L"Offset");
        m_compositionSpotLight.StopAnimation(L"OuterConeAngle");
    }
}
