// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BitmapSource_Partial.h"
#include "SvgImageSource.g.h"
#include "DXamlAsyncBase.h"
#include <fwd/windows.storage.h>

namespace DirectUI
{
    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      The async operation to be used with SvgImageSource::SetSourceAsync
    //
    //------------------------------------------------------------------------
    class SvgImageSourceSetSourceAsyncOperation:
        public DXamlAsyncBaseImpl<wf::IAsyncOperationCompletedHandler<xaml_imaging::SvgImageSourceLoadStatus>,
                                  wf::IAsyncOperation<xaml_imaging::SvgImageSourceLoadStatus>,
                                  Microsoft::WRL::AsyncCausalityOptions<SvgImageSourceSetSourceAsyncOperationName>>,
        public ICoreAsyncOperation<xaml_imaging::SvgImageSourceLoadStatus>
    {
        InspectableClass(wf::IAsyncOperation<xaml_imaging::SvgImageSourceLoadStatus>::z_get_rc_name_impl(), BaseTrust);

    public:

        SvgImageSourceSetSourceAsyncOperation();

        // wf::IAsyncOperation<TRESULT> forwarders
        IFACEMETHOD(put_Completed)(_In_ wf::IAsyncOperationCompletedHandler<xaml_imaging::SvgImageSourceLoadStatus> *pCompletedHandler) override
        {
            return __super::PutOnComplete(pCompletedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  wf::IAsyncOperationCompletedHandler<xaml_imaging::SvgImageSourceLoadStatus> **ppCompletedHandler) override
        {
            return AsyncBase::GetOnComplete(ppCompletedHandler);
        }

        IFACEMETHOD(GetResults) (_Outptr_result_maybenull_ xaml_imaging::SvgImageSourceLoadStatus *results) override {*results = m_status; return S_OK;}

        // SvgImageSourceSetSourceAsyncOperation overrides
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

        void CoreSetError(HRESULT hr) override;
        _Check_return_ HRESULT CoreSetResults(xaml_imaging::SvgImageSourceLoadStatus results) override {m_status = results; return S_OK;}
    private:
        xaml_imaging::SvgImageSourceLoadStatus m_status;
    };

    PARTIAL_CLASS(SvgImageSource)
    {
        friend class SvgImageSourceFactory;

    public:
        ~SvgImageSource() override;

        _Check_return_ HRESULT SetSourceAsyncImpl(_In_ wsts::IRandomAccessStream* pStreamSource, _Outptr_ wf::IAsyncOperation<xaml_imaging::SvgImageSourceLoadStatus>** returnValue);
        _Check_return_ HRESULT SetSourceShared(_In_ wsts::IRandomAccessStream* pStreamSource);

    protected:
        _Check_return_ HRESULT OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *pOperation, _In_ wf::AsyncStatus status);

    private:
        void CancelSetSourceAsyncActions();

        // Used to assign unique ids to AsyncActions
        static ULONG z_ulUniqueAsyncActionId;

        ctl::ComPtr<wsts::IStreamReadOperation> m_spStreamReadOperation;
        ctl::ComPtr<wf::IAsyncInfo> m_spStreamReadOperationAsyncInfo;

        // We can only have one AsyncAction outstanding per SvgImageSource.  In some ways the best design
        // would be for SvgImageSource to actually implement IAsyncAction, but we have a shared implementation
        // that we can pick up easily.  We create new ones each time we start an Async operation, but they
        // don't get cleaned up immediately when the operation completes.  This is consistent with thinking of SvgImageSource
        // as implementing AsyncAction and avoids a complex circular lifetime issue with the action pointing to the SvgImageSource
        // and the SvgImageSource pointing to the Action.  Since the Action does nothing except maintain the Async state machine
        // and refers to no expensive resources, this is fine for now.
        Microsoft::WRL::ComPtr<SvgImageSourceSetSourceAsyncOperation> m_spSetSourceAsyncOperation;

        // Some implementations of IRandomAccessStream::ReadAsync do not ref the stream they are reading, so
        // we ref it temporarily to keep it from going away out from under us
        ctl::ComPtr<wsts::IRandomAccessStream> m_spStream;
    };
}
