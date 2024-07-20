// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implementation of 3D general transforms.

#pragma once

#include "InternalTransform.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(InternalTransform)
    {
    public:
        IFACEMETHOD(get_InverseCore)(_Outptr_ xaml_media::IGeneralTransform** ppValue) override;
        IFACEMETHOD(TransformBoundsCore)(_In_ wf::Rect rect, _Out_ wf::Rect* pReturnValue) override;
        IFACEMETHOD(TryTransformCore)(_In_ wf::Point inPoint, _Out_ wf::Point* pOutPoint, _Out_ BOOLEAN* pReturnValue) override;
    };
}
