// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SoftwareBitmapSource.g.h"
#include "DXamlAsyncBase.h"

namespace DirectUI
{
    PARTIAL_CLASS(SoftwareBitmapSource)
    {
    public:
        SoftwareBitmapSource() = default;

        ~SoftwareBitmapSource() override = default;
        
        // IClosable implementation
        IFACEMETHOD(Close()) override;

        IFACEMETHOD(SetBitmapAsyncImpl)(
            _In_ wgri::ISoftwareBitmap* pSoftwareBitmap,
            _Outptr_ wf::IAsyncAction** ppReturnValue
            );

    private:
        // Used to assign unique ids to AsyncActions
        static ULONG z_ulUniqueAsyncActionId;

        // We can only have one AsyncAction outstanding per SoftwareBitmapSource.  Implemented
        // similarly to BitmapSource class.
        Microsoft::WRL::ComPtr<SoftwareBitmapSourceSetSourceAsyncAction> m_spSetSourceAsyncAction;
    };
}
