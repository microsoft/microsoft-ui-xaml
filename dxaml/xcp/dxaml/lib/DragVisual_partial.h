// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DependentAsyncWorker.h"
#include "DXamlAsyncBase.h"

namespace DirectUI
{
    class ImageBrush;
    class RenderTargetBitmap;

    extern __declspec(selectany) const WCHAR DragVisualRenderAsyncName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.DragVisual.RenderAsync";
    typedef DXamlAsyncActionBase<Microsoft::WRL::AsyncCausalityOptions<DragVisualRenderAsyncName>> DragVisualRenderAsyncAction;

    enum DragVisualContentType
    {
        VisualFromDataPackage,
        VisualFromDraggedUIElement,
        VisualFromSoftwareBitmap,
        VisualFromBitmapImage
    };

    interface __declspec(uuid("2add598c-c032-4539-9960-f4f395789e53")) IDragVisual : IInspectable
    {};

    class DragVisual : public ctl::WeakReferenceSource, public IDragVisual
    {
    public:
        HRESULT Initialize(_In_ xaml::IUIElement* pUIElement);
        HRESULT Initialize(_In_ xaml_imaging::IBitmapImage* pBitmapImage);
        HRESULT Initialize(_In_ wgri::ISoftwareBitmap* pSoftwareBitmap);

        using ctl::WeakReferenceSource::Initialize;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(IDragVisual)))
            {
                *ppObject = static_cast<IDragVisual*>(this);
            }
            else
            {
                RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

        wf::Size GetSize() { return m_size; }

        _Check_return_ HRESULT RenderAsync(_Outptr_ wf::IAsyncAction** pRenderAsyncAction);

        // Returns true if the underlying RenderTargetBitmap and ImageBrush is ready
        bool IsRendered() { return m_isRendered; }

        _Check_return_ HRESULT GetSoftwareBitmap(_COM_Outptr_ wgri::ISoftwareBitmap** ppSoftwareBitmap);

    protected:

    private:
        _Check_return_ HRESULT OnRenderCompleted();
        _Check_return_ HRESULT OnImageFailed();
        _Check_return_ HRESULT OnImageOpened();
        _Check_return_ HRESULT RetrieveImageSize();
        _Check_return_ HRESULT CreateSoftwareBitmap();

        ctl::ComPtr<xaml_media::IImageBrush> m_spImageBrush;
        ctl::ComPtr<xaml_imaging::IRenderTargetBitmap> m_spRenderTargetBitmap;
        ctl::ComPtr<xaml_imaging::IBitmapImage> m_spBitmapImage;
        ctl::ComPtr<wgri::ISoftwareBitmap> m_spSoftwareBitmap;
        ctl::ComPtr<xaml::IUIElement> m_spUIElement;
        ctl::ComPtr<wsts::IBuffer> m_spBuffer;
        wrl::ComPtr<DragVisualRenderAsyncAction> m_spRenderAsyncAction;
        wf::Size m_size{};
        bool m_isRendered = false;
        DragVisualContentType m_dragVisualContentType = DragVisualContentType::VisualFromDataPackage;

        static ULONG z_ulUniqueAsyncActionId;
    };
}
