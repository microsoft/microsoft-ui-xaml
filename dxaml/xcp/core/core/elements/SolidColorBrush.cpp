// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimeMgr.h"
#include <CColor.h>
#include <GraphicsUtility.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the base brush class.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSolidColorBrush::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = S_OK;

    CSolidColorBrush* _this = new CSolidColorBrush(pCreate->m_pCore);
    CDependencyObject *pTemp = nullptr;
    IFC(ValidateAndInit(_this, &pTemp));

    if ( pCreate->m_value.GetType() == valueString || pCreate->m_value.GetType() == valueColor)
    {
        IFC(_this->FromStringOrColor(pCreate));
    }
   *ppObject = pTemp;
    _this = nullptr;

Cleanup:
    if(pTemp) delete _this;
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of a brush object from string or color
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSolidColorBrush::FromStringOrColor(
        _In_ CREATEPARAMETERS *pCreate
        )
{
    ASSERT(pCreate->m_value.GetType() == valueString || pCreate->m_value.GetType() == valueColor);

    HRESULT hr = S_OK;
    CDependencyObject *prgb = nullptr;

    //Parse the color from the string

    if (pCreate->m_value.GetType() == valueString)
    {
        CValue value;
        IFC(CColor::Create(&prgb, pCreate));
        value.WrapObjectNoRef(prgb);
        IFC(SetValue(GetContentProperty(), value));
    }
    else
    {
        // TODO: Can we directly set it into field?
        IFC(SetValue(GetContentProperty(), pCreate->m_value));
    }

Cleanup:
    ReleaseInterface(prgb);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures a PAL brush exists and is up-to-date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSolidColorBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams &renderParams
    )
{
    IPALAcceleratedSolidColorBrush *pPALSolidColorBrush = reinterpret_cast<IPALAcceleratedSolidColorBrush *>(m_pPALBrush);

    if (!pPALSolidColorBrush || pPALSolidColorBrush->GetColor() != m_rgb)
    {
        IFC_RETURN(CreateAcceleratedBrush(renderParams, &m_pPALBrush));
    }

    IFC_RETURN(CBrush::UpdateAcceleratedBrush(renderParams));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a D2D brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSolidColorBrush::CreateAcceleratedBrush(
    _In_ const D2DRenderParams& renderParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedBrush *pPALBrush = nullptr;

    IFC(renderParams.GetD2DRenderTarget()->CreateSolidColorBrush(
        m_rgb,
        GetOpacity(),
        &pPALBrush
        ));

    ReplaceInterface(*ppBrush, pPALBrush);

Cleanup:
    ReleaseInterface(pPALBrush);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the print brush corresponding to this brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSolidColorBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    return CreateAcceleratedBrush(printParams, ppBrush);
}

