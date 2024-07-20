// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DirectConnectedAnimationConfiguration_partial.h"
#include "Corep.h"
//#include "EasingFunctionBase.g.h"
#include "WindowRenderTarget.h"
#include "DCompTreeHost.h"

using namespace DirectUI;

static const INT64 s_DefaultDuration = 200 * 10000; // 150ms
static const wfn::Vector2 s_ControlPoint1 = { 0.1f, 0.9f }; // Control points for deceleration easing function.
static const wfn::Vector2 s_ControlPoint2 = { 0.2f, 1.0f };

_Check_return_ HRESULT DirectUI::DirectConnectedAnimationConfiguration::GetDefaultEasingFunction(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** value)
{
    auto renderTarget = DXamlCore::GetCurrent()->GetHandle()->NWGetWindowRenderTarget();
    Microsoft::WRL::ComPtr<WUComp::ICubicBezierEasingFunction> cubicBezier;
    Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> easingFunction;
    IFC_RETURN(renderTarget->GetDCompTreeHost()->GetEasingFunctionStatics()->CreateCubicBezierEasingFunction(renderTarget->GetDCompTreeHost()->GetCompositor(), s_ControlPoint1, s_ControlPoint2, &cubicBezier));
    IFC_RETURN(cubicBezier.As(&easingFunction));
    *value = easingFunction.Detach();
    return S_OK;
}

const wf::TimeSpan DirectUI::DirectConnectedAnimationConfiguration::GetDefaultDuration()
{
    wf::TimeSpan duration = { s_DefaultDuration };
    return duration;
}
