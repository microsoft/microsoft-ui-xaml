// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InternalTransform.g.h"

using namespace DirectUI;

IFACEMETHODIMP
InternalTransform::get_InverseCore(_Outptr_ IGeneralTransform** ppValue)
{
    HRESULT hr = S_OK;

    CGeneralTransform* pGeneralTransform = NULL;
    DependencyObject* pDO = NULL;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT(pCore);
    IFCEXPECT(ppValue);

    IFC(CoreImports::InternalTransform_Inverse(static_cast<CInternalTransform*>(GetHandle()), &pGeneralTransform));

    IFC(pCore->GetPeer(pGeneralTransform, KnownTypeIndex::InternalTransform, &pDO));
    IFC(ctl::do_query_interface(*ppValue, ctl::as_iinspectable(pDO)));

Cleanup:
    ReleaseInterface(pGeneralTransform);
    ctl::release_interface(pDO);
    RRETURN(hr);
}

IFACEMETHODIMP
InternalTransform::TransformBoundsCore(_In_ wf::Rect rect, _Out_ wf::Rect* pReturnValue)
{
    HRESULT hr = S_OK;
    XRECTF xrect = {};
    XRECTF xrect2 = {};

    IFCPTR(pReturnValue);

    xrect.X = rect.X;
    xrect.Y = rect.Y;
    xrect.Width = rect.Width;
    xrect.Height = rect.Height;

    IFC(CoreImports::InternalTransform_TransformBounds(GetHandle(), xrect, &xrect2));

    pReturnValue->X = xrect2.X;
    pReturnValue->Y = xrect2.Y;
    pReturnValue->Width = xrect2.Width;
    pReturnValue->Height = xrect2.Height;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
InternalTransform::TryTransformCore(_In_ wf::Point inPoint, _Out_ wf::Point* pOutPoint, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    XPOINTF xpoint = {};
    XPOINTF xpoint2 = {};

    IFCPTR(pOutPoint);
    IFCPTR(pReturnValue);

    xpoint.x = inPoint.X;
    xpoint.y = inPoint.Y;

    if (SUCCEEDED(CoreImports::InternalTransform_Transform(GetHandle(), xpoint, &xpoint2)))
    {
        pOutPoint->X = xpoint2.x;
        pOutPoint->Y = xpoint2.y;
        *pReturnValue = TRUE;
    }
    else
    {
        pOutPoint->X = 0.0;
        pOutPoint->Y = 0.0;
        *pReturnValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}
