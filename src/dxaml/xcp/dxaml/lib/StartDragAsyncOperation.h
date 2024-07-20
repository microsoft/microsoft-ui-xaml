// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DXamlAsyncBase.h"
#include "microsoft.ui.input.dragdrop.h"

namespace DirectUI
{
    using StartDragAsyncOperationCompletedHandler = wf::IAsyncOperationCompletedHandler<wadt::DataPackageOperation>;

    class StartDragAsyncOperation :
        public DXamlAsyncBaseImpl<StartDragAsyncOperationCompletedHandler,
                                  wf::IAsyncOperation<wadt::DataPackageOperation>>
    {
        InspectableClass(wf::IAsyncOperation<wadt::DataPackageOperation>::z_get_rc_name_impl(), BaseTrust);

    public:
        // wf::IAsyncOperation<DataPackageOperation> forwarders
        IFACEMETHOD(put_Completed)(_In_ StartDragAsyncOperationCompletedHandler *pCompletedHandler) override
        {
            return __super::PutOnComplete(pCompletedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  StartDragAsyncOperationCompletedHandler **ppCompletedHandler) override
        {
            return __super::GetOnComplete(ppCompletedHandler);
        }

        IFACEMETHOD(Cancel)(void) override;

        IFACEMETHOD(GetResults) (
            _Inout_ wadt::DataPackageOperation *results) override;

        STDMETHOD(RuntimeClassInitialize)(
            _In_opt_ ctl::WeakRefPtr wrUIElement,
            _In_ mui::DragDrop::IDragOperation* dragOperation)
        {
            auto id = z_ulUniqueAsyncActionId;

            InterlockedIncrement(&z_ulUniqueAsyncActionId);

            AsyncBase::put_Id(id);
            m_spDragOperation = dragOperation;
            m_wrUIElement = wrUIElement;

            return S_OK;
        }

        _Check_return_ HRESULT SetVisual(_In_ bool hasAnchorPoint, _In_ wf::Point anchorPoint, _In_opt_ DragVisual* dragVisual);
        _Check_return_ HRESULT StartIfNeeded(_In_ CDependencyObject* anchorElement, _In_ ixp::IPointerPoint* pointerPoint);
        _Check_return_ HRESULT DoCancel();

        ~StartDragAsyncOperation();

    protected:
        // wf::IAsyncAction overrides
        _Check_return_ HRESULT OnStart(void) override
        {
            DXamlCore::GetCurrent()->SetIsWinRTDndOperationInProgress(true);
            m_started = true;
            return S_OK;
        }

    private:
        _Check_return_ HRESULT OnRenderCompleted(_In_opt_ wf::IAsyncAction*, wf::AsyncStatus status);
        _Check_return_ HRESULT StartIfNeededImpl(_In_opt_ CDependencyObject* anchorElement, _In_ ixp::IPointerPoint* pointerPoint);

        // Used to assign unique ids to AsyncActions
        static ULONG z_ulUniqueAsyncActionId;
        bool m_hasAnchorPoint = false;
        bool m_started = false;
        bool m_deferVisual = true;
        wf::Point m_anchorPoint;
        ctl::ComPtr<DragVisual> m_spDragVisual;
        ctl::ComPtr<mui::DragDrop::IDragOperation> m_spDragOperation;
        ctl::ComPtr<IAsyncInfo> m_spCoreDragOperationAsyncInfo;
        wadt::DataPackageOperation m_result = wadt::DataPackageOperation::DataPackageOperation_None;
        ctl::WeakRefPtr m_wrUIElement;
    };
}
