// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "fwd/microsoft.ui.input.dragdrop.h"
#include "PointerPointTransform.h"

namespace DirectUI
{
    class RaiseDragDropEventAsyncOperation;
    struct PendingDragDropActionData;
#if XCP_MONITOR
    using PendingDragDropActionQueue = std::deque<std::unique_ptr<PendingDragDropActionData>, ::XcpAllocation::LeakIgnoringAllocator<std::unique_ptr<PendingDragDropActionData>>>;
#else
    using PendingDragDropActionQueue = std::deque<std::unique_ptr<PendingDragDropActionData>>;
#endif


    class DropOperationTarget :
        public mui::DragDrop::IDropOperationTarget,
        public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(DropOperationTarget, ctl::ComBase)
            INTERFACE_ENTRY(DropOperationTarget, mui::DragDrop::IDropOperationTarget)
        END_INTERFACE_MAP(DropOperationTarget, ctl::ComBase)

    public:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        HRESULT Initialize(_In_ CXamlIslandRoot* hitTestRootIsland);

        // IDropOperationTarget Methods
        IFACEMETHOD(EnterAsync)(
            _In_ mui::DragDrop::IDragInfo* pDragInfo,
            _In_ mui::DragDrop::IDragUIOverride* pDragUIOverride,
            _Deref_out_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue) override;
        IFACEMETHOD(OverAsync)(
            _In_ mui::DragDrop::IDragInfo* pDragInfo,
            _In_ mui::DragDrop::IDragUIOverride* pDragUIOverride,
            _Deref_out_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue) override;
        IFACEMETHOD(LeaveAsync)(
            _In_ mui::DragDrop::IDragInfo* pDragInfo,
            _Deref_out_ wf::IAsyncAction** ppReturnValue) override;
        IFACEMETHOD(DropAsync)(
            _In_ mui::DragDrop::IDragInfo* pDragInfo,
            _Deref_out_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue) override;

        HRESULT RaiseDragDropEventActionAsync(
            _In_ DragDropMessageType type,
            _In_ mui::DragDrop::IDragInfo* pDragInfo,
            _In_opt_ RaiseDragDropEventAsyncOperation* pRaiseDragDropEventAsyncOperation,
            _Deref_out_ wf::IAsyncAction** ppReturnValue);

        _Check_return_ HRESULT RaiseDragDropEventOperationAsync(
            _In_ DragDropMessageType type,
            _In_ mui::DragDrop::IDragInfo* pDragInfo,
            _In_opt_ mui::DragDrop::IDragUIOverride* pDragUIOverride,
            _Deref_out_ wf::IAsyncOperation<wadt::DataPackageOperation>** returnValue);

        _Check_return_ HRESULT ProcessDragDropEventAction(
             _In_ DragDropMessageType type,
            _In_ mui::DragDrop::IDragInfo* pDragInfo,
            _In_opt_ RaiseDragDropEventAsyncOperation* pRaiseDragDropEventAsyncOperation = nullptr);

        void CheckIfAcceptedOperationShouldBeReset(_In_ IInspectable* sourceAsInspectable);
        void SetAcceptedOperationSetterUIElement(_In_ IInspectable* sourceAsInspectable);
        void SetAcceptedOperation(_In_ IInspectable* sourceAsInspectable,
                                  _In_ wadt::DataPackageOperation acceptedOperation);

        void SetDragDropPointTransform(ctl::ComPtr<DirectUI::PointerPointTransform>& transform)
        {
            m_dragDropPointTransform = transform;
        }

        HRESULT RaisePendingDragDropEventAction();
        HRESULT QueuePendingActionCallback();

    private:
        // Indicates that we are currently processing an action and may receive reentrant messages
        static thread_local bool tls_actionInProgress;
        // List of actions that have been queued due to reentrancy
        static thread_local std::unique_ptr<PendingDragDropActionQueue> tls_pendingActions;
        // The currently active drop operation target
        static thread_local ctl::ComPtr<DropOperationTarget> tls_activeDropOperationTarget;

        // When this is set to true, the Accepted Operation will be reset
        bool m_shouldResetAcceptedOperation = false;
        // Cache the UIElement which sets the AcceptedOperation
        ctl::WeakRefPtr m_wrAcceptedOperationSetterIInspectable;
        // and keep the value to set in in next DragOver call
        DirectUI::DataPackageOperation m_acceptedOperation = DirectUI::DataPackageOperation::DataPackageOperation_None;
        // Provided in the case of Islands, to record the root element to hit test from
        xref::weakref_ptr<CXamlIslandRoot> m_hitTestRootIslandWeak;
        // Point transform for windowed components (e.g. popups).
        ctl::ComPtr<DirectUI::PointerPointTransform> m_dragDropPointTransform;
    };
}


