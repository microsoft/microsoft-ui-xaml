// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include <windows.foundation.numerics.h>

class CCoreServices;

// Holds the timeline spline point for keyframe spline interpolation.
// This uses a cubic Bezier with fixed start (0,0) and end (1,1) points.
class CKeySpline final : public CDependencyObject
{
private:
    CKeySpline(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CKeySpline()    // !!! FOR UNIT TESTING ONLY !!!
        : CKeySpline(nullptr)
    {}
#endif

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CKeySpline>::Index;
    }

    float GetSplineProgress(float linearProgress);

    void CopyKeySplineProperties(_Inout_ CKeySpline *pClone) const;

    wfn::Vector2 GetControlPoint1();
    wfn::Vector2 GetControlPoint2();

private:
    _Check_return_ HRESULT InitFromString(
        _In_ XUINT32 cString,
        _In_reads_(cString) const WCHAR *pString
        );

    _Check_return_ HRESULT InvokeImpl(
        _In_ const CDependencyProperty *pdp,
        _In_opt_ CDependencyObject *pNamescopeOwner) override;

    static _Check_return_ HRESULT IsControlPointValid(_In_ XPOINTF *pControlPoint)
    {
        HRESULT hr = S_OK;

        if ( pControlPoint->x < 0
          || pControlPoint->y < 0
          || pControlPoint->x > 1
          || pControlPoint->y > 1 )
        {
            hr = E_FAIL;
        }

        RRETURN(hr);
    }

// Computational methods

//
//  NOTE on math: most of the theory was gathered from:
//
//    Eric W. Weisstein. "Bernstein Polynomial." From MathWorld--A Wolfram Web Resource.
//    http://mathworld.wolfram.com/BernsteinPolynomial.html
//        B(0,3) = (1-t)^3
//        B(1,3) = 3(1-t)^2 t
//        B(2,3) = 3(1-t) t^2
//        B(3,3) = t^3
//
//  Also, the bulk of this code was ported from WPF as-is, there is potential for
//  optimization here.
//

    XFLOAT ComputeSplineValueY(_In_ XFLOAT rT) const
    {

    // Precompute some convenient values
        XFLOAT rOneMinusT = 1.0f - rT;
        XFLOAT rOneMinusTSquared = rOneMinusT * rOneMinusT;
        XFLOAT rTSquared = rT * rT;
        XFLOAT rTCubed = rT * rTSquared;

    // Compute Bezier coefficients
        XFLOAT rB13 = 3.0f * rOneMinusTSquared * rT;
        XFLOAT rB23 = 3.0f * rOneMinusT * rTSquared;
        XFLOAT rB33 = rTCubed;

    // Compute Y as a function of T
    // NOTE that P0=0,0 and P3=1,1 so this simplified
        XFLOAT rY = rB13 * m_ControlPoint1.y
                  + rB23 * m_ControlPoint2.y
                  + rB33;

        return rY;
    }

    void GetXAndDeltaX(_In_ XFLOAT rT,
                       _Out_ XFLOAT *rX,
                       _Out_ XFLOAT *rDeltaX) const
    {
    // Precompute some convenient values
        XFLOAT rOneMinusT = 1.0f - rT;
        XFLOAT rOneMinusTSquared = rOneMinusT * rOneMinusT;
        XFLOAT rTSquared = rT * rT;
        XFLOAT rTCubed = rT * rTSquared;

    // Compute Bezier coefficients
        XFLOAT rB13 = 3.0f * rOneMinusTSquared * rT;
        XFLOAT rB23 = 3.0f * rOneMinusT * rTSquared;
        XFLOAT rB33 = rTCubed;

    // Compute X as a function of T
    // NOTE that P0=0,0 and P3=1,1 so this simplified
        *rX = rB13 * m_ControlPoint1.x
            + rB23 * m_ControlPoint2.x
            + rB33;

    // Compute DeltaX as a function of T --> DT/DX
        XFLOAT rBX = 3.0f * m_ControlPoint1.x;
        XFLOAT rCX = 3.0f * m_ControlPoint2.x;
        XFLOAT rBX_CX = 2.0f * (rCX - rBX);
        XFLOAT r3CX = 3.0f - rCX;

        *rDeltaX = rOneMinusTSquared * rBX
                 + rT * rOneMinusT * rBX_CX
                 + r3CX * rTSquared;
    }

public:
    XPOINTF m_ControlPoint1 = {};
    XPOINTF m_ControlPoint2 = { 1.0f, 1.0f };
    XFLOAT m_rLastT         = 0.0f;
};
