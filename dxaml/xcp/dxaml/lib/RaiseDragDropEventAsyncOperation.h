// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DropOperationTarget.h"
#include "DragVisual_partial.h"
#include "IDragOperationDeferralTarget.g.h"
#include "DXamlAsyncBase.h"
#include "fwd/microsoft.ui.input.dragdrop.h"

namespace DirectUI
{
    typedef wf::IAsyncOperationCompletedHandler<wadt::DataPackageOperation> RaiseDragDropEventAsyncOperationCompletedHandler;
    class RaiseDragDropEventAsyncOperation :
        public DXamlAsyncBaseImpl<RaiseDragDropEventAsyncOperationCompletedHandler,
                                  wf::IAsyncOperation<wadt::DataPackageOperation>,
                                  Microsoft::WRL::AsyncOptions<>,
                                  IDragOperationDeferralTarget>
    {
        InspectableClass(wf::IAsyncOperation<wadt::DataPackageOperation>::z_get_rc_name_impl(), BaseTrust);

    public:

        ~RaiseDragDropEventAsyncOperation();

        // wf::IAsyncOperation<DataPackageOperation> forwarders
        IFACEMETHOD(put_Completed)(_In_ RaiseDragDropEventAsyncOperationCompletedHandler *pCompletedHandler) override
        {
            return __super::PutOnComplete(pCompletedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  RaiseDragDropEventAsyncOperationCompletedHandler **ppCompletedHandler) override
        {
            return __super::GetOnComplete(ppCompletedHandler);
        }

        IFACEMETHOD(GetResults) (
            _Inout_ wadt::DataPackageOperation *result) override
        {
            *result = m_result;
            return S_OK;
        }

        STDMETHOD(RuntimeClassInitialize)(
            _In_ DragDropMessageType type,
            _In_ DropOperationTarget* target,
            _In_ mui::DragDrop::IDragInfo* dragInfo,
            _In_opt_ mui::DragDrop::IDragUIOverride* dragUI)
        {
            auto id = z_ulUniqueAsyncActionId;

            InterlockedIncrement(&z_ulUniqueAsyncActionId);

            AsyncBase::put_Id(id);

            m_type = type;
            m_spDragInfo = dragInfo;
            m_spDragUIOverride = dragUI;
            m_spDropTarget = target;

            return S_OK;
        }

        _Check_return_ HRESULT SetupCustomDragVisual();

       void SetAcceptedOperation(
            _In_ wadt::DataPackageOperation acceptedOperation)
        {
            m_result = acceptedOperation;
        }

        void SetDragVisual(
            _In_ DragVisual* pDragVisual)
        {
            m_spDragVisual = pDragVisual;
        }
        void SetDragVisual(
            _In_ DragVisual* pDragVisual,
            _In_ wf::Point anchorPoint)
        {
            m_spDragVisual = pDragVisual;
            m_anchorPoint = anchorPoint;
            m_hasAnchorPoint = true;
        }

        _Check_return_ HRESULT get_DragInfo(
            _Outptr_ mui::DragDrop::IDragInfo** ppDragInfo)
        {
            return m_spDragInfo.CopyTo(ppDragInfo);
        }

        _Check_return_ HRESULT get_DragUIOverride(
            _Outptr_ mui::DragDrop::IDragUIOverride** ppDragUI)
        {
            return m_spDragUIOverride.CopyTo(ppDragUI);
        }

        IFACEMETHOD(DeferralAdded)()
        {
            m_deferralCount++;
            return S_OK;
        }

        IFACEMETHOD(DeferralCompleted)()
        {
            ASSERT(m_deferralCount > 0);
            // if m_wasDeferred == false, it means that the deferral has been completed
            // before OnRaiseDragDropCompleted was called, which means that this
            // method will run normally as if no deferral had been taken, and therefore
            // we should no-op here.
            // It would be typically be the case if the app writes some code as
            // args->GetDeferral()->Complete();
            // in the event handler...
            if ((0 == --m_deferralCount) && m_wasDeferred)
            {
                IFC_RETURN(OnStart());
            }
            return S_OK;
        }

        IFACEMETHOD(SetAcceptedOperation)(_In_ IInspectable* source,
                                          _In_ wadt::DataPackageOperation acceptedOperation);

        bool IsDeferred() const
        {
            return m_deferralCount > 0;
        }

        // This RPInvoke is called in DragLeave to check if the UIElement we are leaving had set the AcceptedOperation
        static _Check_return_ HRESULT CheckIfAcceptedOperationShouldBeCleared(_In_ IInspectable* pOperation, _In_opt_ CDependencyObject* pSource);

        // And the associated setter
        static _Check_return_ HRESULT SetAcceptedOperationSetterUIElement(_In_ IInspectable* pOperation, _In_opt_ CDependencyObject* pSource);

    protected:
        // wf::IAsyncAction overrides
        _Check_return_ HRESULT OnStart(void) override;

    private:
        _Check_return_ HRESULT OnCustomVisualSetUp(_In_opt_ wf::IAsyncAction*, _In_ wf::AsyncStatus status);
        _Check_return_ HRESULT OnRaiseDragDropCompleted(_In_opt_ wf::IAsyncAction*, wf::AsyncStatus status);

        // Used to assign unique ids to AsyncActions
        static ULONG z_ulUniqueAsyncActionId;

        Microsoft::WRL::ComPtr<mui::DragDrop::IDragInfo> m_spDragInfo;
        Microsoft::WRL::ComPtr<mui::DragDrop::IDragUIOverride> m_spDragUIOverride;
        ctl::ComPtr<DropOperationTarget> m_spDropTarget;
        ctl::ComPtr<DragVisual> m_spDragVisual;
        wf::Point m_anchorPoint;
        bool m_hasAnchorPoint = false;
        bool m_wasDeferred = false;
        wadt::DataPackageOperation m_result = wadt::DataPackageOperation::DataPackageOperation_None;
        DragDropMessageType m_type{};
        UINT32 m_deferralCount = 0;
        // Used to hold a ref to self while invoking completion handler
        Microsoft::WRL::ComPtr<RaiseDragDropEventAsyncOperation> m_spThis;
    };
}
