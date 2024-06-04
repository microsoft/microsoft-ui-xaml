// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlCompositionBrush.h"

// Update the printing brush
_Check_return_ HRESULT
CXamlCompositionBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams &renderParams
)
{
    auto pPALSolidColorBrushNoRef = static_cast<IPALAcceleratedSolidColorBrush*>(m_pPALBrush);

    if (!pPALSolidColorBrushNoRef || pPALSolidColorBrushNoRef->GetColor() != m_fallbackColor)
    {
        ReleaseInterface(m_pPALBrush);
        IFC_RETURN(CreateAcceleratedBrush(renderParams, &m_pPALBrush));
    }
    IFC_RETURN(CBrush::UpdateAcceleratedBrush(renderParams));

    return S_OK;
}

// Create the printing brush
_Check_return_ HRESULT CXamlCompositionBrush::CreateAcceleratedBrush(
    _In_ const D2DRenderParams& renderParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
)
{
    xref_ptr<IPALAcceleratedBrush> palBrush;
    IFC_RETURN(renderParams.GetD2DRenderTarget()->CreateSolidColorBrush(m_fallbackColor, GetOpacity(), palBrush.ReleaseAndGetAddressOf()));
    *ppBrush = palBrush.detach();

    return S_OK;
}

_Check_return_ HRESULT CXamlCompositionBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
)
{
    return CreateAcceleratedBrush(printParams, ppBrush);
}