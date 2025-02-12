// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

namespace DirectUI
{
    class MatrixFactory:
        public ABI::Microsoft::UI::Xaml::Media::IMatrixHelperStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(MatrixFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(MatrixFactory, ABI::Microsoft::UI::Xaml::Media::IMatrixHelperStatics)
        END_INTERFACE_MAP(MatrixFactory, ctl::AbstractActivationFactory)

    public:
        // Extension methods.
        IFACEMETHOD(GetIsIdentity)(ABI::Microsoft::UI::Xaml::Media::Matrix target, _Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(Transform)(ABI::Microsoft::UI::Xaml::Media::Matrix target, ABI::Windows::Foundation::Point point, _Out_ ABI::Windows::Foundation::Point* pReturnValue) override;

        // Static properties.
        IFACEMETHOD(get_Identity)(_Out_ ABI::Microsoft::UI::Xaml::Media::Matrix* pValue) override;

        // Static methods.
        IFACEMETHOD(FromElements)(DOUBLE m11, DOUBLE m12, DOUBLE m21, DOUBLE m22, DOUBLE offsetX, DOUBLE offsetY, _Out_ ABI::Microsoft::UI::Xaml::Media::Matrix* pReturnValue) override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
    };
}

#include "Matrix_Partial.h"
