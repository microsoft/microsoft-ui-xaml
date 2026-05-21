// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <Geometry.h>
#include <RectangleGeometry.h>


CGeometry::~CGeometry()
{
}

_Check_return_ HRESULT CGeometry::GetBounds(_Out_ XRECTF_RB* pBounds)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::GetWidenedBounds(
    _In_ const CPlainPen& pen,
    _Out_ XRECTF_RB* pBounds)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::HitTestFill(
    _In_ const XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::HitTestFill(
    _In_ const HitTestPolygon& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::ClipToFill(
    _Inout_ XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::ClipToFill(
    _Inout_ HitTestPolygon& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::HitTestStroke(
    _In_ const XPOINTF& target,
    _In_ const CPlainPen& pen,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::HitTestStroke(
    _In_ const HitTestPolygon& target,
    _In_ const CPlainPen& pen,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::VisitSink(_In_ IPALGeometrySink* pSink)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CGeometry::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    _COM_Outptr_result_maybenull_ IPALAcceleratedGeometry** ppGeometry)
{
    *ppGeometry = nullptr;
    return E_NOTIMPL;
}

_Check_return_ HRESULT CRectangleGeometry::GetBounds(_Out_ XRECTF_RB* pBounds)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CRectangleGeometry::GetWidenedBounds(
    _In_ const CPlainPen& pen,
    _Out_ XRECTF_RB* pBounds)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CRectangleGeometry::HitTestFill(
    _In_ const XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CRectangleGeometry::HitTestFill(
    _In_ const HitTestPolygon& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CRectangleGeometry::VisitSinkInternal(_In_ IPALGeometrySink* pSink)
{
    return E_NOTIMPL;
}

bool CRectangleGeometry::CanBeAccelerated()
{
    return false;
}

_Check_return_ HRESULT CRectangleGeometry::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    _COM_Outptr_result_maybenull_ IPALAcceleratedGeometry** ppGeometry)
{
    *ppGeometry = nullptr;
    return E_NOTIMPL;
}

WUComp::ICompositionGeometry* CRectangleGeometry::GetCompositionGeometry(
    _In_ VisualContentRenderer* renderer)
{
    return nullptr;
}

_Check_return_ HRESULT CGeometry::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    _In_ LeaveParams params)
{
    return E_NOTIMPL;
}
void CGeometry::ReleaseDCompResources()
{
    return;
}
