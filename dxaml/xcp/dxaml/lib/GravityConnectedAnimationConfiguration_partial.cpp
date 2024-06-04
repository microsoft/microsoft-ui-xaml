// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GravityConnectedAnimationConfiguration_partial.h"
#include "DCompTreeHost.h"

using namespace DirectUI;

static const float s_dippedZ = 32.0f;
static const float s_dippedY = 80.0f;
static const float s_dippedScale = 1.1f;
static const float animationPeakTime = .66f;
static const wfn::Vector2 s_InControlPoint1 = { 0.35f, 0.0f }; // Control points for rising to peak easing function.
static const wfn::Vector2 s_InControlPoint2 = { 0.55f, 1.0f };
static const wfn::Vector2 s_OutControlPoint1 = { 0.45f, 0.0f }; // Control points for declining from peak easing function.
static const wfn::Vector2 s_OutControlPoint2 = { 0.55f, 1.0f };

_Check_return_ HRESULT GravityConnectedAnimationConfiguration::get_IsShadowEnabledImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isShadowEnabled;
    return S_OK;
}

_Check_return_ HRESULT GravityConnectedAnimationConfiguration::put_IsShadowEnabledImpl(_In_ BOOLEAN value)
{
    m_isShadowEnabled = value;
    return S_OK;
}

_Check_return_ HRESULT GravityConnectedAnimationConfiguration::GetEffectPropertySet(_In_ wfn::Vector3 scaleFactors, _Out_ WUComp::ICompositionPropertySet** effectPropertySet)
{
    wf::TimeSpan ts;
    IFC_RETURN(GetEffectiveDuration(&ts));

    WUComp::ICompositor* compositor = DXamlCore::GetCurrent()->GetHandle()->GetCompositor();
    ixp::ICompositionEasingFunctionStatics* easingFunctionStatics = DXamlCore::GetCurrent()->GetHandle()->GetDCompTreeHost()->GetEasingFunctionStatics();

    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> propertySet;
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> compObject;
    IFC_RETURN(compositor->CreatePropertySet(&propertySet));
    IFC_RETURN(propertySet.As(&compObject));

    Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> easingFunctionIn;
    Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> easingFunctionOut;

    {
        Microsoft::WRL::ComPtr<WUComp::ICubicBezierEasingFunction> cubicBezier;
        IFC_RETURN(easingFunctionStatics->CreateCubicBezierEasingFunction(compositor, s_InControlPoint1, s_InControlPoint2, &cubicBezier));
        IFC_RETURN(cubicBezier.As(&easingFunctionIn));
    }

    {
        Microsoft::WRL::ComPtr<WUComp::ICubicBezierEasingFunction> cubicBezier;
        IFC_RETURN(easingFunctionStatics->CreateCubicBezierEasingFunction(compositor, s_OutControlPoint1, s_OutControlPoint2, &cubicBezier));
        IFC_RETURN(cubicBezier.As(&easingFunctionOut));
    }

    // Animate the offset
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> animation;
        Microsoft::WRL::ComPtr<WUComp::IVector3KeyFrameAnimation> vector3Animation;
        Microsoft::WRL::ComPtr<WUComp::IKeyFrameAnimation> keyFrameAnimation;

        wfn::Vector3 naturalOffset = { 0.0f, 0.0f, 0.0f };
        wfn::Vector3 dippedOffset = { 0.0f, s_dippedY, s_dippedZ };
        IFC_RETURN(propertySet->InsertVector3(wrl_wrappers::HStringReference(L"Offset").Get(), naturalOffset));
        IFC_RETURN(compositor->CreateVector3KeyFrameAnimation(vector3Animation.GetAddressOf()));

        IFC_RETURN(vector3Animation.As(&keyFrameAnimation));
        IFC_RETURN(keyFrameAnimation->put_Duration(ts));

        IFC_RETURN(vector3Animation->InsertKeyFrame(0.0f, naturalOffset));
        IFC_RETURN(vector3Animation->InsertKeyFrameWithEasingFunction(animationPeakTime, dippedOffset, easingFunctionIn.Get()));
        IFC_RETURN(vector3Animation->InsertKeyFrameWithEasingFunction(1.0f, naturalOffset, easingFunctionOut.Get()));
        IFC_RETURN(vector3Animation.Get()->QueryInterface(IID_PPV_ARGS(&animation)));
        IFC_RETURN(compObject->StartAnimation(wrl_wrappers::HStringReference(L"Offset").Get(), animation.Get()));

    }

    // Animate the scale (but only if some scale animation isn't already occurring)
    if (scaleFactors.X == 1.0f && scaleFactors.Y == 1.0f)
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> animation;
        Microsoft::WRL::ComPtr<WUComp::IVector3KeyFrameAnimation> vector3Animation;
        Microsoft::WRL::ComPtr<WUComp::IKeyFrameAnimation> keyFrameAnimation;

        wfn::Vector3 naturalScale = { 1.0f, 1.0f, 1.0f };
        wfn::Vector3 dippedScale = { s_dippedScale, s_dippedScale, 1.0f };
        IFC_RETURN(propertySet->InsertVector3(wrl_wrappers::HStringReference(L"Scale").Get(), naturalScale));

        IFC_RETURN(compositor->CreateVector3KeyFrameAnimation(vector3Animation.GetAddressOf()));

        IFC_RETURN(vector3Animation.As(&keyFrameAnimation));
        IFC_RETURN(keyFrameAnimation->put_Duration(ts));

        IFC_RETURN(vector3Animation->InsertKeyFrame(0.0f, naturalScale));
        IFC_RETURN(vector3Animation->InsertKeyFrameWithEasingFunction(animationPeakTime, dippedScale, easingFunctionIn.Get()));
        IFC_RETURN(vector3Animation->InsertKeyFrameWithEasingFunction(1.0f, naturalScale, easingFunctionOut.Get()));
        IFC_RETURN(vector3Animation.Get()->QueryInterface(IID_PPV_ARGS(&animation)));
        IFC_RETURN(compObject->StartAnimation(wrl_wrappers::HStringReference(L"Scale").Get(), animation.Get()));
    }

    *effectPropertySet = propertySet.Detach();

    return S_OK;
}


