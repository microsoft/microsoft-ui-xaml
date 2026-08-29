// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DragUI.g.h"
#include "DragVisual_partial.h"

namespace DirectUI
{


    PARTIAL_CLASS(DragUI)
    {
    public:
        // IDragUI implementation
        _Check_return_ HRESULT SetContentFromBitmapImageImpl(_In_ xaml_imaging::IBitmapImage* pBitmapImage);

        _Check_return_ HRESULT SetContentFromBitmapImageWithAnchorPointImpl(_In_ xaml_imaging::IBitmapImage* pBitmapImage, _In_ wf::Point anchorPoint);

        _Check_return_ HRESULT SetContentFromSoftwareBitmapImpl(
            _In_ wgri::ISoftwareBitmap* pSoftwareBitmap);

        _Check_return_ HRESULT SetContentFromSoftwareBitmapWithAnchorPointImpl(
            _In_ wgri::ISoftwareBitmap* pSoftwareBitmap,
            _In_ wf::Point anchorPoint);

        _Check_return_ HRESULT SetContentFromDataPackageImpl();

        // Internal methods
        _Check_return_ HRESULT CreateDragVisual(_COM_Outptr_ DragVisual** ppDragVisual,
                                                _Out_ bool *pHasAnchorPoint,
                                                _Out_ wf::Point* pAnchorPoint);

        // Some calls are not valid anymore once the Core Drag Operation has been started
        void SetCoreDragOperationStarted(_In_ bool value)
        {
            m_isCoreDragOperationStarted = value;
        }
        bool GetContentFromDataPackage() const
        {
            return m_dragVisualContentType == DragVisualContentType::VisualFromDataPackage;
        }
    private:
        DragVisualContentType m_dragVisualContentType = DragVisualContentType::VisualFromDraggedUIElement;
        bool m_hasAnchorPoint = false;
        bool m_isCoreDragOperationStarted = false;
        wf::Point m_anchorPoint{};
        ctl::ComPtr<xaml_imaging::IBitmapImage> m_spBitmapImage;
        ctl::ComPtr<wgri::ISoftwareBitmap> m_spSoftwareBitmap;
    };
}
