// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CScaleTransform::SetValue(_In_ const SetValueParams& args)
{
    // Need to catch this case because the change to Auto for WIdth/Height
    // on FrameworkElements could potentially trickle down to these transforms
    // and making them invalid.
    if (args.m_pDP != nullptr &&
        (args.m_pDP->GetIndex() == KnownPropertyIndex::ScaleTransform_CenterX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::ScaleTransform_CenterY ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::ScaleTransform_ScaleX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::ScaleTransform_ScaleY) &&
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

void CScaleTransform::GetTransform(_Out_ CMILMatrix *pMatrix)
{
    pMatrix->SetM11(m_eScaleX);
    pMatrix->SetM12(0.0f);
    pMatrix->SetM21(0.0f);
    pMatrix->SetM22(m_eScaleY);
    pMatrix->SetDx(m_ptCenter.x * (1.0f - m_eScaleX));
    pMatrix->SetDy(m_ptCenter.y * (1.0f - m_eScaleY));
}
