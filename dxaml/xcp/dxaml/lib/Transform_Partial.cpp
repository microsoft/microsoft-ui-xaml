// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Transform.g.h"

using namespace DirectUI;

_Check_return_ HRESULT GeneralTransform::TransformPointImpl(_In_ wf::Point point, _Out_ wf::Point* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN success = FALSE;

    IFCPTR(pReturnValue);

    IFC(CheckThread());

    IFC(TryTransform(point, pReturnValue, &success));

    if (!success)
    {
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Transform::get_InverseCore(_Outptr_ IGeneralTransform** ppValue)
{
    HRESULT hr = S_OK;

    CGeneralTransform* pGeneralTransform = NULL;
    ctl::ComPtr<DependencyObject> spDO;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT(pCore);
    IFCEXPECT(ppValue);

    IFC(CoreImports::Transform_Inverse(static_cast<CTransform*>(GetHandle()), &pGeneralTransform));

    IFC(pCore->GetPeer(pGeneralTransform, KnownTypeIndex::MatrixTransform, &spDO));
    IFC(spDO.CopyTo(ppValue));

Cleanup:
    ReleaseInterface(pGeneralTransform);
    RRETURN(hr);
}

IFACEMETHODIMP
Transform::TransformBoundsCore(_In_ wf::Rect rect, _Out_ wf::Rect* pReturnValue)
{
    HRESULT hr = S_OK;
    XRECTF xrect = {};
    XRECTF xrect2 = {};

    IFCPTR(pReturnValue);

    xrect.X = rect.X;
    xrect.Y = rect.Y;
    xrect.Width = rect.Width;
    xrect.Height = rect.Height;

    IFC(CoreImports::Transform_TransformBounds(GetHandle(), xrect, &xrect2));

    pReturnValue->X = xrect2.X;
    pReturnValue->Y = xrect2.Y;
    pReturnValue->Width = xrect2.Width;
    pReturnValue->Height = xrect2.Height;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Transform::TryTransformCore(_In_ wf::Point inPoint, _Out_ wf::Point* pOutPoint, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    XPOINTF xpoint = {};
    XPOINTF xpoint2 = {};

    IFCPTR(pOutPoint);
    IFCPTR(pReturnValue);

    xpoint.x = inPoint.X;
    xpoint.y = inPoint.Y;

    if (SUCCEEDED(CoreImports::Transform_Transform(GetHandle(), xpoint, &xpoint2)))
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

