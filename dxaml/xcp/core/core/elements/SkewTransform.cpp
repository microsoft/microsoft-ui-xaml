// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CSkewTransform::SetValue(_In_ const SetValueParams& args)
{
    // Need to catch this case because the change to Auto for Width/Height
    // on FrameworkElements could potentially trickle down to these transforms
    // and making them invalid.
    if (args.m_pDP != nullptr &&
        (args.m_pDP->GetIndex() == KnownPropertyIndex::SkewTransform_CenterX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::SkewTransform_CenterY) &&
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

void CSkewTransform::GetTransform(_Out_ CMILMatrix *pMatrix)
{
    BuildSkewMatrix(pMatrix, m_eAngleX, m_eAngleY, m_ptCenter);
}
