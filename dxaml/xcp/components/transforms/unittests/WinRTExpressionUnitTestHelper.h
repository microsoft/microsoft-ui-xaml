// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include "WinRTExpressionConversionContext.h"
#include "WinRTLocalExpressionBuilder.h"
#include <MockDComp-UnitTestHelpers.h>


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

class WinRTExpressionUnitTestHelper
{
public:
    // static WinRTLocalExpressionBuilder MakeWinRTLocalExpressionBuilder();

    static WinRTExpressionConversionContext MakeContext();

    static void VerifyTranslateTransform(
        _In_ WUComp::IExpressionAnimation* pExpression,
        _In_ const wchar_t* pszExpectedExpressionString,
        float x,
        float y,
        _In_opt_ WUComp::ICompositionAnimation* pXAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pYAnimation
        );

    static void VerifyScaleTransform(
        _In_ WUComp::IExpressionAnimation* pExpression,
        _In_ const wchar_t* pszExpectedExpressionString,
        _In_ bool hasCenterPoint,
        float centerX,
        float centerY,
        float scaleX,
        float scaleY,
        _In_opt_ WUComp::ICompositionAnimation* pCenterXAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pCenterYAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pScaleXAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pScaleYAnimation
        );

    static void VerifyRotateTransform(
        _In_ WUComp::IExpressionAnimation* pExpression,
        _In_ const wchar_t* pszExpectedExpressionString,
        _In_ bool hasCenterPoint,
        float centerX,
        float centerY,
        float angle,
        _In_opt_ WUComp::ICompositionAnimation* pCenterXAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pCenterYAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pAngleAnimation
        );

    static void VerifySkewTransform(
        _In_ WUComp::IExpressionAnimation* pExpression,
        _In_ const wchar_t* pszExpectedExpressionString,
        _In_ bool hasCenterPoint,
        float centerX,
        float centerY,
        float angleX,
        float angleY,
        _In_opt_ WUComp::ICompositionAnimation* pCenterXAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pCenterYAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pAngleXAnimation,
        _In_opt_ WUComp::ICompositionAnimation* pAngleYAnimation
        );

    static void VerifyMatrixTransform(
        _In_ WUComp::IExpressionAnimation* pExpression,
        _In_ const wchar_t* pszExpectedExpressionString,
        float m11,
        float m12,
        float m21,
        float m22,
        float m31,
        float m32
        );

    static void VerifyRenderTransformOrigin(
        _In_ WUComp::IExpressionAnimation* pExpression,
        float adjustX,
        float adjustY);

    static void VerifyExpressionString(_In_ WUComp::IExpressionAnimation* expr, _In_ const wchar_t* pszExpected);

    static void VerifyScalarProperty(
        _In_ std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap,
        _In_ const wchar_t* pszPropertyName,
        _In_ float expectedValue,
        _In_opt_ WUComp::ICompositionAnimation* animation
        );

    static std::map<std::wstring, MockDComp::PropertyInfo>& GetPropertyInfoMap(_In_ WUComp::IExpressionAnimation* expr);

    static wfn::Matrix4x4 GetMatrix4x4Identity()
    {
        return { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    }

    static bool IsIdentity(const wfn::Matrix4x4& m1)
    {
        return (m1.M11 == 1 && m1.M12 == 0 && m1.M13 == 0 && m1.M14 == 0 &&
                m1.M21 == 0 && m1.M22 == 1 && m1.M23 == 0 && m1.M24 == 0 &&
                m1.M31 == 0 && m1.M32 == 0 && m1.M33 == 1 && m1.M34 == 0 &&
                m1.M41 == 0 && m1.M42 == 0 && m1.M43 == 0 && m1.M44 == 1);
    }

    static bool IsIdentity(const wfn::Matrix3x2& m1)
    {
        return (m1.M11 == 1 && m1.M12 == 0 &&
                m1.M21 == 0 && m1.M22 == 1 &&
                m1.M31 == 0 && m1.M32 == 0);
    }
};

class ScalarKFAHelper
{
public:
    ScalarKFAHelper(WinRTExpressionConversionContext context)
    {
        context.GetCompositorNoRef()->CreateScalarKeyFrameAnimation(m_scalarKFA1.ReleaseAndGetAddressOf());
        m_scalarKFA1.As<WUComp::ICompositionAnimation>(&m_spAnim1);
    }

    operator WUComp::ICompositionAnimation*() { return m_spAnim1.Get(); }

private:
    Microsoft::WRL::ComPtr<WUComp::IScalarKeyFrameAnimation> m_scalarKFA1;
    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> m_spAnim1;
};

} } } } } }
