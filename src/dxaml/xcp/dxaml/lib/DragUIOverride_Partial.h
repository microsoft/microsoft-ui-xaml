// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DragUIOverride.g.h"
#include "RaiseDragDropEventAsyncOperation.h"
#include "fwd/microsoft.ui.input.dragdrop.h"

namespace DirectUI
{
    PARTIAL_CLASS(DragUIOverride)
    {
    public:
        // pRaiseDragDropEventAsyncOperation and pOriginalSource will be null
        // when called from DragLeaveAsync or DropAsync
        // We provide a no-op DragUIOVerride in this case because we share
        // the same DragEventArgs for all events
        _Check_return_ HRESULT Initialize(
            _In_opt_ wf::IAsyncOperation<wadt::DataPackageOperation>* pRaiseDragDropEventAsyncOperation,
            _In_opt_ IInspectable* pOriginalSource,
            _In_ bool updateCoreDragUI);
        using ctl::WeakReferenceSource::Initialize;

        _Check_return_ HRESULT SetContentFromBitmapImageImpl(
            _In_ xaml_imaging::IBitmapImage* pBitmapImage);

        _Check_return_ HRESULT SetContentFromBitmapImageWithAnchorPointImpl(
            _In_ xaml_imaging::IBitmapImage* pBitmapImage,
            _In_ wf::Point anchorPoint);

        _Check_return_ HRESULT SetContentFromSoftwareBitmapImpl(
            _In_ wgri::ISoftwareBitmap* pSoftwareBitmap);

        _Check_return_ HRESULT SetContentFromSoftwareBitmapWithAnchorPointImpl(
            _In_ wgri::ISoftwareBitmap* pSoftwareBitmap,
            _In_ wf::Point anchorPoint);

        _Check_return_ HRESULT ClearImpl();

        _Check_return_ HRESULT get_CaptionImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT put_CaptionImpl(_In_opt_ HSTRING value);
        _Check_return_ HRESULT get_IsContentVisibleImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsContentVisibleImpl(_In_ BOOLEAN value);
        _Check_return_ HRESULT get_IsCaptionVisibleImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsCaptionVisibleImpl(_In_ BOOLEAN value);
        _Check_return_ HRESULT get_IsGlyphVisibleImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsGlyphVisibleImpl(_In_ BOOLEAN value);

        ~DragUIOverride();

    private:
        // As we expose this class for XAML Events (DragLeave and Drop) where it is not backed up
        // by core, we keep the values locally
        BOOLEAN m_isContentVisible = TRUE;
        BOOLEAN m_isCaptionVisible = TRUE;
        BOOLEAN m_isGlyphVisible = TRUE;
        wrl_wrappers::HString m_caption;
        bool m_updateCoreDragUI = false;

        ctl::ComPtr<IInspectable> m_spOriginalSource;
        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> m_spRaiseDragDropEventAsyncOperation;
        ctl::ComPtr<mui::DragDrop::IDragUIOverride> m_spDragUIOverride;

        // Register the original source as responsible for the override in order to
        // reset all overrides when we exit this source
        void RegisterOverride();
    };
}
