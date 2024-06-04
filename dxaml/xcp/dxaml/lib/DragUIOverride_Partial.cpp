// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragEventArgs.g.h"
#include "DragDropInternal.h"
#include "microsoft.ui.input.dragdrop.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DelayRelease(IInspectable* inspectable);

DragUIOverride::~DragUIOverride()
{
    // We need to ensure that the final release of the winrt drag info instance is delayed.
    // If not, the final release could also initiante another drag operation in the
    // same UI thread, but becuase Xaml is draining the clean up queue,
    // then OnReentrancyProtectedWindowMessage could be trigger and the app would crash.
    IFCFAILFAST(DelayRelease(m_spDragUIOverride.Get()));
}

_Check_return_ HRESULT DragUIOverride::Initialize(
    _In_opt_ wf::IAsyncOperation<wadt::DataPackageOperation>* pRaiseDragDropEventAsyncOperation,
    _In_opt_ IInspectable* pOriginalSource,
    _In_ bool updateCoreDragUI)
{
    // pRaiseDragDropEventAsyncOperation and pOriginalSource will be null
    // when called from DragLeaveAsync or DropAsync
    // We provide a no-op DragUIOVerride in this case because we share
    // the same DragEventArgs for all events
    m_spOriginalSource = pOriginalSource;
    m_updateCoreDragUI = updateCoreDragUI;
    m_spRaiseDragDropEventAsyncOperation = pRaiseDragDropEventAsyncOperation;
    if (m_spRaiseDragDropEventAsyncOperation)
    {
        IFC_RETURN(m_spRaiseDragDropEventAsyncOperation.Cast<RaiseDragDropEventAsyncOperation>()->get_DragUIOverride(m_spDragUIOverride.ReleaseAndGetAddressOf()));

        // if we are following a DragLeave, we might have to reset the Visual to avoid collision
        // between our changes and the previous ones
        if (DXamlCore::GetCurrent()->GetDragDrop()->ShouldClearCustomVisual() && m_spDragUIOverride)
        {
            IFC_RETURN(m_spDragUIOverride->Clear());
        }

        if (!m_updateCoreDragUI)
        {
            // We will be reading and writing locally and hence we initialize the local values correctly
            boolean visible = false;
            IFC_RETURN(m_spDragUIOverride->get_Caption(m_caption.ReleaseAndGetAddressOf()));
            IFC_RETURN(m_spDragUIOverride->get_IsCaptionVisible(&visible));
            m_isCaptionVisible = !!visible;
            IFC_RETURN(m_spDragUIOverride->get_IsContentVisible(&visible));
            m_isContentVisible = !!visible;
            IFC_RETURN(m_spDragUIOverride->get_IsGlyphVisible(&visible));
            m_isGlyphVisible = !!visible;
        }
    }
    return S_OK;
}

// Register the original source as responsible for the override in order to
// reset all overrides when we exit this source
void DragUIOverride::RegisterOverride()
{
    if (m_spOriginalSource)
    {
        DXamlCore::GetCurrent()->GetDragDrop()->SetCustomVisualSetterUIElement(m_spOriginalSource.Get());
    }
}

_Check_return_ HRESULT DragUIOverride::SetContentFromBitmapImageImpl(
    _In_ xaml_imaging::IBitmapImage* pBitmapImage)
{
    if (m_updateCoreDragUI && m_spRaiseDragDropEventAsyncOperation)
    {
        ctl::ComPtr<DragVisual> spDragVisual;
        IFC_RETURN(ctl::make<DragVisual>(pBitmapImage, &spDragVisual));
        m_spRaiseDragDropEventAsyncOperation.Cast<RaiseDragDropEventAsyncOperation>()->SetDragVisual(spDragVisual.Get());
        RegisterOverride();
    }
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::SetContentFromBitmapImageWithAnchorPointImpl(
    _In_ xaml_imaging::IBitmapImage* pBitmapImage,
    _In_ wf::Point anchorPoint)
{
    if (m_updateCoreDragUI && m_spRaiseDragDropEventAsyncOperation)
    {
        ctl::ComPtr<DragVisual> spDragVisual;
        IFC_RETURN(ctl::make<DragVisual>(pBitmapImage, &spDragVisual));
        m_spRaiseDragDropEventAsyncOperation.Cast<RaiseDragDropEventAsyncOperation>()->SetDragVisual(spDragVisual.Get(), anchorPoint);
        RegisterOverride();
    }
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::SetContentFromSoftwareBitmapImpl(
    _In_ wgri::ISoftwareBitmap* pSoftwareBitmap)
{
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        IFC_RETURN(m_spDragUIOverride->SetContentFromSoftwareBitmap(pSoftwareBitmap));
        RegisterOverride();
    }
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::SetContentFromSoftwareBitmapWithAnchorPointImpl(
    _In_ wgri::ISoftwareBitmap* pSoftwareBitmap,
    _In_ wf::Point anchorPoint)
{
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        IFC_RETURN(m_spDragUIOverride->SetContentFromSoftwareBitmap2(pSoftwareBitmap, anchorPoint));
        RegisterOverride();
    }
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::get_CaptionImpl(_Out_ HSTRING* pValue)
{
    if (m_spDragUIOverride)
    {
        return m_spDragUIOverride->get_Caption(pValue);
    }
    else
    {
        return m_caption.CopyTo(pValue);
    }
}

_Check_return_ HRESULT DragUIOverride::put_CaptionImpl(_In_opt_ HSTRING value)
{
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        IFC_RETURN(m_spDragUIOverride->put_Caption(value));
    }
    else
    {
        IFC_RETURN(m_caption.Set(value));
    }
    RegisterOverride();
    return S_OK;

}
_Check_return_ HRESULT DragUIOverride::get_IsContentVisibleImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isContentVisible;
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        boolean visible = false;
        IFC_RETURN(m_spDragUIOverride->get_IsContentVisible(&visible));
        *pValue = !!visible;
    }
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::put_IsContentVisibleImpl(_In_ BOOLEAN value)
{
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        IFC_RETURN(m_spDragUIOverride->put_IsContentVisible(!!value));
    }
    else
    {
        m_isContentVisible = value;
    }
    RegisterOverride();
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::get_IsCaptionVisibleImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isCaptionVisible;
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {

        boolean visible = false;
        IFC_RETURN(m_spDragUIOverride->get_IsCaptionVisible(&visible));
        *pValue = !!visible;
    }
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::put_IsCaptionVisibleImpl(_In_ BOOLEAN value)
{
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        IFC_RETURN(m_spDragUIOverride->put_IsCaptionVisible(!!value));
    }
    else
    {
        m_isCaptionVisible = value;
    }
    RegisterOverride();
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::get_IsGlyphVisibleImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isGlyphVisible;
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        boolean visible = false;
        IFC_RETURN(m_spDragUIOverride->get_IsGlyphVisible(&visible));
        *pValue = !!visible;
    }
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::put_IsGlyphVisibleImpl(_In_ BOOLEAN value)
{
    if (m_updateCoreDragUI && m_spDragUIOverride)
    {
        IFC_RETURN(m_spDragUIOverride->put_IsGlyphVisible(!!value));
    }
    else
    {
        m_isGlyphVisible = value;
    }
    RegisterOverride();
    return S_OK;
}

_Check_return_ HRESULT DragUIOverride::ClearImpl()
{
    if (m_spDragUIOverride)
    {
        IFC_RETURN(m_spDragUIOverride->Clear());
    }
    return S_OK;
}
