// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LinearGradientBrush.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(LinearGradientBrush)
    {
        // Grant friend access to LinearGradientBrushFactory so it can
        // use the EndPointFromAngle helper method.
        friend class LinearGradientBrushFactory;
    public:

        _Check_return_ HRESULT get_TranslationImpl(_Out_ wfn::Vector2* translation);
        _Check_return_ HRESULT put_TranslationImpl(const wfn::Vector2& translation);
        _Check_return_ HRESULT get_RotationImpl(_Out_ DOUBLE* rotation);
        _Check_return_ HRESULT put_RotationImpl(DOUBLE rotation);
        _Check_return_ HRESULT get_ScaleImpl(_Out_ wfn::Vector2* scale);
        _Check_return_ HRESULT put_ScaleImpl(const wfn::Vector2& scale);
        _Check_return_ HRESULT get_TransformMatrixImpl(_Out_ wfn::Matrix3x2* transformMatrix);
        _Check_return_ HRESULT put_TransformMatrixImpl(const wfn::Matrix3x2& transformMatrix);
        _Check_return_ HRESULT get_CenterPointImpl(_Out_ wfn::Vector2* centerPoint);
        _Check_return_ HRESULT put_CenterPointImpl(const wfn::Vector2& centerPoint);

        // Animated properties are not public and so don't need a way to cross over to core, but
        // the methods get generated automatically when we define them in the CodeGen sources to
        // get KnownPropertyIndexes (which are used for simple property storage).
        _Check_return_ HRESULT get_AnimatedTranslationImpl(_Out_ wfn::Vector2* translation) { return E_NOTIMPL; }
        _Check_return_ HRESULT put_AnimatedTranslationImpl(const wfn::Vector2& translation) { return E_NOTIMPL; }
        _Check_return_ HRESULT get_AnimatedRotationImpl(_Out_ DOUBLE* rotation) { return E_NOTIMPL; }
        _Check_return_ HRESULT put_AnimatedRotationImpl(DOUBLE rotation) { return E_NOTIMPL; }
        _Check_return_ HRESULT get_AnimatedScaleImpl(_Out_ wfn::Vector2* scale) { return E_NOTIMPL; }
        _Check_return_ HRESULT put_AnimatedScaleImpl(const wfn::Vector2& scale) { return E_NOTIMPL; }
        _Check_return_ HRESULT get_AnimatedTransformMatrixImpl(_Out_ wfn::Matrix3x2* transformMatrix) { return E_NOTIMPL; }
        _Check_return_ HRESULT put_AnimatedTransformMatrixImpl(const wfn::Matrix3x2& transformMatrix) { return E_NOTIMPL; }
        _Check_return_ HRESULT get_AnimatedCenterPointImpl(_Out_ wfn::Vector2* centerPoint) { return E_NOTIMPL; }
        _Check_return_ HRESULT put_AnimatedCenterPointImpl(const wfn::Vector2& centerPoint) { return E_NOTIMPL; }
    private:

        static wf::Point EndPointFromAngle(_In_ DOUBLE angle);
        static const DOUBLE EndPointFromAngleMultiplier;
    };
}

