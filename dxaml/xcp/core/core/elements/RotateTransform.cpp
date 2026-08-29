// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinMath.h"

_Check_return_ HRESULT CRotateTransform::SetValue(_In_ const SetValueParams& args)
{
    // Need to catch this case because the change to Auto for WIdth/Height
    // on FrameworkElements could potentially trickle down to these transforms
    // and making them invalid.
    if (args.m_pDP &&
        (args.m_pDP->GetIndex() == KnownPropertyIndex::RotateTransform_CenterX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::RotateTransform_CenterY) &&
        args.m_value.IsFloatingPoint() &&
        _isnan(FLOAT(args.m_value.AsDouble())))
    {
        CValue zeroValue;
        zeroValue.SetFloat(0.0f);
        return CTransform::SetValue(SetValueParams(args.m_pDP, zeroValue));
    }
    else
    {
        return CTransform::SetValue(args);
    }
}

void CRotateTransform::GetTransform(_Out_ CMILMatrix *pMatrix)
{
    XFLOAT eCos, eSin, clampedAngle;

    // Due to errors inherent in binary floating-point arithmetic, answers to
    //  some common angles aren't *exactly* what we would expect.  Hard-code the
    //  precise answers so we don't have to do fuzzy "close enough" comparisons
    //  elsewhere in the codebase.

    // If the angle is outside of the range of 0-360, convert to its equivalent
    //  within that range so things like 540 degrees will still work with the
    //  special cases below.
    clampedAngle = ClampAngle360(m_eAngle);

    // Zero does not need a special case, because zero degrees will be converted
    //  to precisely zero radians.
    if( clampedAngle == 90.0f )
    {
        eCos = 0.0f;
        eSin = 1.0f;
    }
    else if( clampedAngle == 180.0f )
    {
        eCos = -1.0f;
        eSin = 0.0f;
    }
    else if( clampedAngle == 270.0f )
    {
        eCos = 0.0f;
        eSin = -1.0f;
    }
    else
    {
        eCos = MathCosDegrees(m_eAngle);
        eSin = MathSinDegrees(m_eAngle);
    }

    pMatrix->SetM11(eCos);
    pMatrix->SetM12(eSin);
    pMatrix->SetM21(-eSin);
    pMatrix->SetM22(eCos);
    pMatrix->SetDx(m_ptCenter.x * (1.0f - eCos) + (m_ptCenter.y * eSin));
    pMatrix->SetDy(m_ptCenter.y * (1.0f - eCos) - (m_ptCenter.x * eSin));
}
