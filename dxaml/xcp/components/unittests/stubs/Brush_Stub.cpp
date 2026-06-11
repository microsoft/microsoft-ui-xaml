// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

// Needed for private DComp interfaces
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINTHRESHOLD

#include "SolidColorBrush.h"
#include "LinearGradientBrush.h"

CBrush::CBrush(_In_ const CBrush& original, _Out_ HRESULT& hr)
    : CMultiParentShareableDependencyObject(original, hr)
{
    hr = S_OK;
}

CBrush::~CBrush()
{
}

_Check_return_ HRESULT CBrush::GetValue(_In_ const CDependencyProperty *pdp, _Out_ CValue *pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CBrush::HitTestBrushClipInLocalSpace(
    _In_ const XRECTF *pGeometryRenderBounds,
    _In_ const XPOINTF& target,
    _Out_ bool* pIsHit
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CBrush::HitTestBrushClipInLocalSpace(
    _In_ const XRECTF *pGeometryRenderBounds,
    _In_ const HitTestPolygon& target,
    _Out_ bool* pIsHit
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CBrush::D2DEnsureDeviceIndependentResources(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform,
    _In_ const XRECTF_RB *pBrushBounds,
    _Inout_ AcceleratedBrushParams *pPALBrushParams
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams& renderParams
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    return E_NOTIMPL;
}

DCompTreeHost* CBrush::GetDCompTreeHost()
{
    return nullptr;
}

_Check_return_ HRESULT CSolidColorBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams &renderParams
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CSolidColorBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    return E_NOTIMPL;
}

#include "XamlCompositionBrush.h"

_Check_return_ HRESULT CXamlCompositionBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams &renderParams
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CXamlCompositionBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    return E_NOTIMPL;
}

CGradientBrush::~CGradientBrush()
{
}

#pragma warning(suppress: 6387) // test function, only case initializing with nullptr is allowed
CLinearGradientBrush::CLinearGradientBrush(
    _In_ const CLinearGradientBrush& original,
    _Out_ HRESULT& hr )
    : CGradientBrush(nullptr)
{
    hr = S_OK;
}

_Check_return_ HRESULT CLinearGradientBrush::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    _In_ LeaveParams params)
{
    return E_NOTIMPL;
}

void CLinearGradientBrush::CleanupDeviceRelatedResourcesRecursive(
    _In_ bool cleanupDComp)
{
}

_Check_return_ HRESULT CLinearGradientBrush::D2DEnsureDeviceIndependentResources(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform,
    _In_ const XRECTF_RB *pBrushBounds,
    _Inout_ AcceleratedBrushParams *pPALBrushParams)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CLinearGradientBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams& renderParams)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CLinearGradientBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush)
{
    return E_NOTIMPL;
}
