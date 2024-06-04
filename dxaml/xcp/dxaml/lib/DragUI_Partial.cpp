// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragUI_partial.h"
#include "BitmapImage.g.h"
#include "ImageBrush.g.h"
#include "RenderTargetBitmap.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// IDragUI implementation
_Check_return_ HRESULT
DragUI::SetContentFromBitmapImageImpl(_In_ xaml_imaging::IBitmapImage* pBitmapImage)
{
    m_spBitmapImage = pBitmapImage;
    m_dragVisualContentType = DragVisualContentType::VisualFromBitmapImage;
    m_hasAnchorPoint = false;
    return S_OK;
}

_Check_return_ HRESULT
DragUI::SetContentFromBitmapImageWithAnchorPointImpl(
    _In_ xaml_imaging::IBitmapImage* pBitmapImage,
    _In_ wf::Point anchorPoint)
{
    IFC_RETURN(SetContentFromBitmapImageImpl(pBitmapImage));
    m_anchorPoint = anchorPoint;
    m_hasAnchorPoint = true;
    return S_OK;
}

_Check_return_ HRESULT
DragUI::SetContentFromSoftwareBitmapImpl(
_In_ wgri::ISoftwareBitmap* pSoftwareBitmap)
{
    m_spSoftwareBitmap = pSoftwareBitmap;
    m_hasAnchorPoint = false;
    m_dragVisualContentType = DragVisualContentType::VisualFromSoftwareBitmap;
    return S_OK;
}

_Check_return_ HRESULT
DragUI::SetContentFromSoftwareBitmapWithAnchorPointImpl(
    _In_ wgri::ISoftwareBitmap* pSoftwareBitmap,
    _In_ wf::Point anchorPoint)
{
    IFC_RETURN(SetContentFromSoftwareBitmapImpl(pSoftwareBitmap));
    m_anchorPoint = anchorPoint;
    m_hasAnchorPoint = true;
    return S_OK;
}

_Check_return_
HRESULT DragUI::SetContentFromDataPackageImpl()
{
    if (m_isCoreDragOperationStarted)
    {
        // This is not allowed anymore
        return E_INVALID_OPERATION;
    }
    else
    {
        m_dragVisualContentType = DragVisualContentType::VisualFromDataPackage;
        m_hasAnchorPoint = false;
        return S_OK;
    }
}

_Check_return_ HRESULT DragUI::CreateDragVisual(
    _COM_Outptr_ DragVisual** ppDragVisual,
    _Out_ bool *pHasAnchorPoint, 
    _Out_ wf::Point* pAnchorPoint)
{
    *ppDragVisual = nullptr;
    *pHasAnchorPoint = false;
    *pAnchorPoint = { 0, 0 };

    ctl::ComPtr<DragVisual> spDragVisual;

    switch (m_dragVisualContentType)
    {
    case DragVisualContentType::VisualFromDataPackage:
    default:
        *ppDragVisual = nullptr;
        return S_OK;
    case DragVisualContentType::VisualFromSoftwareBitmap:
        IFC_RETURN(ctl::make<DragVisual>(m_spSoftwareBitmap.Get(), &spDragVisual));
        break;
    case DragVisualContentType::VisualFromBitmapImage:
        IFC_RETURN(ctl::make<DragVisual>(m_spBitmapImage.Get(), &spDragVisual));
        break;
    }
    *ppDragVisual = spDragVisual.Detach();
    *pHasAnchorPoint = m_hasAnchorPoint;
    if (m_hasAnchorPoint)
    {
        *pAnchorPoint = m_anchorPoint;
    }
    return S_OK;
}
