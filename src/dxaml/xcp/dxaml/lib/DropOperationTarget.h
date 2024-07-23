// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "fwd/microsoft.ui.input.dragdrop.h"

namespace DirectUI
{
    class RaiseDragDropEventAsyncOperation;

    class DropOperationTarget:
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

        void CheckIfAcceptedOperationShouldBeReset(_In_ IInspectable* sourceAsInspectable);
        void SetAcceptedOperationSetterUIElement(_In_ IInspectable* sourceAsInspectable);
        void SetAcceptedOperation(_In_ IInspectable* sourceAsInspectable,
                                  _In_ wadt::DataPackageOperation acceptedOperation);

    private:
        // When this is set to true, the Accepted Operation will be reset
        bool m_shouldResetAcceptedOperation = false;
        // Cache the UIElement which sets the AcceptedOperation
        ctl::WeakRefPtr m_wrAcceptedOperationSetterIInspectable;
        // and keep the value to set in in next DragOver call
        DirectUI::DataPackageOperation m_acceptedOperation = DirectUI::DataPackageOperation::DataPackageOperation_None;
        // Provided in the case of Islands, to record the root element to hit test from
        xref::weakref_ptr<CXamlIslandRoot> m_hitTestRootIslandWeak;
    };
}


