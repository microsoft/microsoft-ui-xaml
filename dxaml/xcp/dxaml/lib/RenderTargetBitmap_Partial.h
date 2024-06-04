// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RenderTargetBitmap.g.h"
#include "DXamlAsyncBase.h"

namespace DirectUI
{
    PARTIAL_CLASS(RenderTargetBitmap)
    {
        friend class RenderTargetBitmapFactory;

    public:
        RenderTargetBitmap();
        ~RenderTargetBitmap() override;

        _Check_return_ HRESULT RenderAsyncImpl(
            _In_opt_ xaml::IUIElement* pElement,
            _Outptr_ wf::IAsyncAction** ppReturnValue);
        _Check_return_ HRESULT RenderToSizeAsyncImpl(
            _In_opt_ xaml::IUIElement* pElement,
            _In_ INT scaledWidth,
            _In_ INT scaledHeight,
            _Outptr_ wf::IAsyncAction** ppReturnValue);
        _Check_return_ HRESULT GetPixelsAsyncImpl(
            _Outptr_ wf::IAsyncOperation<wsts::IBuffer*>** ppReturnValue);

    private:
        static ULONG z_ulUniqueAsyncActionId;
    };

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Implements IBuffer for RenderTargetBitmap::GetPixelsAsync
    //
    //------------------------------------------------------------------------
    class RenderTargetBitmapPixelBuffer :
        public wsts::IBuffer,
        public ::Windows::Storage::Streams::IBufferByteAccess,
        public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(RenderTargetBitmapPixelBuffer, ctl::ComBase)
            INTERFACE_ENTRY(RenderTargetBitmapPixelBuffer, wsts::IBuffer)
            INTERFACE_ENTRY(RenderTargetBitmapPixelBuffer, ::Windows::Storage::Streams::IBufferByteAccess)
        END_INTERFACE_MAP(RenderTargetBitmapPixelBuffer, ctl::ComBase)

    public:
        RenderTargetBitmapPixelBuffer();
        ~RenderTargetBitmapPixelBuffer() override;

        // IBuffer methods
        IFACEMETHOD(get_Capacity)(_Out_ UINT32 *pValue) override;
        IFACEMETHOD(get_Length)(_Out_ UINT32 *pValue) override;
        IFACEMETHOD(put_Length)(_In_ UINT32 value) override;

        // IBufferByteAccess methods
        IFACEMETHOD(Buffer)(_Outptr_ BYTE **ppBuffer) override;

        // Buffer management methods
        _Check_return_ HRESULT InitializeBuffer(_In_ UINT32 length, _In_reads_(length) XBYTE *pBytes);
        void ReleaseBuffer();

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override;

    private:
        XUINT32 m_length;
        _Field_size_(m_length) XBYTE *m_pBuffer;
    };

    extern __declspec(selectany) const WCHAR RenderTargetBitmapRenderAsyncOperationName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Media.Imaging.RenderTargetBitmap.RenderAsync";
    extern __declspec(selectany) const WCHAR RenderTargetBitmapGetPixelsAsyncOperationName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Media.Imaging.RenderTargetBitmap.GetPixelsAsync";

    typedef DXamlAsyncActionBase<Microsoft::WRL::AsyncCausalityOptions<RenderTargetBitmapRenderAsyncOperationName>> RenderTargetBitmapRenderAsyncAction;

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      The async operation to be used with RenderTargetBitmap::GetPixelsAsync
    //
    //------------------------------------------------------------------------
    class RenderTargetBitmapGetPixelsAsyncOperation:
        public DXamlAsyncBaseImpl<wf::IAsyncOperationCompletedHandler<wsts::IBuffer*>,
                                  wf::IAsyncOperation<wsts::IBuffer*>,
                                  Microsoft::WRL::AsyncCausalityOptions<RenderTargetBitmapGetPixelsAsyncOperationName>>,
        public IRenderTargetBitmapGetPixelsAsyncOperation
    {
        InspectableClass(wf::IAsyncOperation<wsts::IBuffer*>::z_get_rc_name_impl(), BaseTrust);

    public:

        RenderTargetBitmapGetPixelsAsyncOperation()
        {
        }

        // wf::IAsyncOperation<TRESULT> forwarders
        IFACEMETHOD(put_Completed)(_In_ wf::IAsyncOperationCompletedHandler<wsts::IBuffer*> *pCompletedHandler) override
        {
            return __super::PutOnComplete(pCompletedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  wf::IAsyncOperationCompletedHandler<wsts::IBuffer*> **ppCompletedHandler) override
        {
            return AsyncBase::GetOnComplete(ppCompletedHandler);
        }

        IFACEMETHOD(GetResults) (_Outptr_result_maybenull_ wsts::IBuffer **results) override;

        // IRenderTargetBitmapGetPixelsAsyncOperation overrides
        // This is the API the our code (Core and DXaml layer) talks to
        bool CoreContinueAsyncAction() override
        {
            return CoreContinueAsyncActionImpl();
        }

        void CoreFireCompletion() override
        {
            CoreFireCompletionImpl();
        }

        void CoreReleaseRef(void) override
        {
            CoreReleaseRefImpl();
        }

        void CoreSetError(HRESULT hr) override
        {
            CoreSetErrorImpl(hr);
        }

        _Check_return_ HRESULT CoreSetResults(RenderTargetBitmapPixelData results) override;
    private:
        ctl::ComPtr<RenderTargetBitmapPixelBuffer> m_spPixelBuffer;
    };
}
