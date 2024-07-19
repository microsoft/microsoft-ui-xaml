// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LinearGradientBrush.h"
#include "LinearGradientBrush.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

const DOUBLE LinearGradientBrush::EndPointFromAngleMultiplier = 0.0055555555555555558 * 3.1415926535897931;

wf::Point LinearGradientBrush::EndPointFromAngle(_In_ DOUBLE angle)
{
    wf::Point point = {};
    XFLOAT radians = static_cast<XFLOAT>(angle * EndPointFromAngleMultiplier);

    point.X = cosf(radians);
    point.Y = sinf(radians);

    return point;
}

_Check_return_ HRESULT LinearGradientBrush::get_TranslationImpl(_Out_ wfn::Vector2* translation)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    *translation = brush->GetTranslation();
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::put_TranslationImpl(const wfn::Vector2& translation)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    brush->SetTranslation(translation);
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::get_RotationImpl(_Out_ DOUBLE* rotation)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    *rotation = brush->GetRotation();
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::put_RotationImpl(DOUBLE rotation)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    brush->SetRotation(rotation);
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::get_ScaleImpl(_Out_ wfn::Vector2* scale)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    *scale = brush->GetScale();
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::put_ScaleImpl(const wfn::Vector2& scale)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    brush->SetScale(scale);
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::get_TransformMatrixImpl(_Out_ wfn::Matrix3x2* transformMatrix)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    *transformMatrix = brush->GetTransformMatrix();
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::put_TransformMatrixImpl(const wfn::Matrix3x2& transformMatrix)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    brush->SetTransformMatrix(transformMatrix);
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::get_CenterPointImpl(_Out_ wfn::Vector2* centerPoint)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    *centerPoint = brush->GetCenterPoint();
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrush::put_CenterPointImpl(const wfn::Vector2& centerPoint)
{
    CLinearGradientBrush* brush = static_cast<CLinearGradientBrush*>(GetHandle());
    brush->SetCenterPoint(centerPoint);
    return S_OK;
}

_Check_return_ HRESULT LinearGradientBrushFactory::CreateInstanceWithGradientStopCollectionAndAngleImpl(
    _In_ wfc::IVector<xaml_media::GradientStop*>* gradientStopCollection,
    _In_ DOUBLE angle,
    _Outptr_ ILinearGradientBrush** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<LinearGradientBrush> spBrush;

    IFC(ctl::make(&spBrush));
    IFC(spBrush->put_GradientStops(gradientStopCollection));
    IFC(spBrush->put_EndPoint(LinearGradientBrush::EndPointFromAngle(angle)));

    *ppInstance = spBrush.Detach();

Cleanup:
    RRETURN(hr);
}

