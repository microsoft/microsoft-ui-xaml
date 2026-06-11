// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DragStartingEventArgs.g.h"
#include "DropOperationTarget.h"
#include "UIElement.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DragStartingEventArgs)
    {
    public:
        DragStartingEventArgs()
        {
            m_allowedOperations = wadt::DataPackageOperation::DataPackageOperation_Copy |
                                  wadt::DataPackageOperation::DataPackageOperation_Move |
                                  wadt::DataPackageOperation::DataPackageOperation_Link;
        }

        _Check_return_ HRESULT SetOperationContext(_In_ wf::Point position,
                                          _In_ wf::IAsyncOperation<wadt::DataPackageOperation> *pStartDragOperation,
                                          _In_ wadt::IDataPackage* pDataPackage,
                                          _In_ ixp::IPointerPoint* pPointerPoint);

        _Check_return_ HRESULT get_DragUIImpl(_COM_Outptr_ xaml::IDragUI** ppValue);
        _Check_return_ HRESULT GetPositionImpl(_In_opt_ xaml::IUIElement* pRelativeTo, _Out_ wf::Point* pReturnValue);
        _Check_return_ HRESULT GetDeferralImpl(_COM_Outptr_ xaml::IDragOperationDeferral** ppReturnValue);

        _Check_return_ HRESULT DeferralAddedImpl();
        _Check_return_ HRESULT DeferralCompletedImpl();
        _Check_return_ HRESULT SetAcceptedOperationImpl(_In_ IInspectable*,_In_ wadt::DataPackageOperation)
        {
            return S_OK;
        }

        _Check_return_ HRESULT StartRaiseEvent(
            _In_ UIElementGenerated::DragStartingEventSourceType* pParent,
            _In_ xaml::IUIElement *pSource,
            _In_ const TrackerPtrVector<wf::ITypedEventHandler<xaml::UIElement*, xaml::DragStartingEventArgs*>> &delegates,
            _In_ const containers::bit_vector& handledTooValues,
            bool shouldStartOperation);

        _Check_return_ HRESULT StartOrUpdateOperation();

    private:
        _Check_return_ HRESULT ResumeRaiseEvent(bool shouldStartOperation);
        _Check_return_ HRESULT SetOperationVisual();

        ctl::ComPtr<xaml::IDragUI> m_spDragUI;
        ctl::ComPtr<xaml::IUIElement> m_spSource;
        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> m_spStartDragAsyncOperation;
        wf::Point m_position{};
        bool m_wasDeferred = false;
        unsigned int m_deferralCount = 0;
        ctl::ComPtr<ixp::IPointerPoint> m_pointerPoint;

        ctl::ComPtr<UIElementGenerated::DragStartingEventSourceType> m_spParent;
        std::vector<UIElementGenerated::DragStartingEventSourceType::HandlerInfo> m_handlers;
        std::vector<UIElementGenerated::DragStartingEventSourceType::HandlerInfo>::iterator m_currentHandler;
    };

    class CDragStartingEventSource : public UIElementGenerated::DragStartingEventSourceType
    {
    public:
        _Check_return_ HRESULT Raise(_In_opt_ xaml::IUIElement* pSource,
            _In_ xaml::IDragStartingEventArgs* pArgs,
            bool shouldStartOperation)
        {
            ctl::ComPtr<xaml::IDragStartingEventArgs> spArgs(pArgs);
            IFC_RETURN(spArgs.Cast<DragStartingEventArgs>()->StartRaiseEvent(this, pSource, m_delegates, m_handledTooValues, shouldStartOperation));
            return S_OK;
        }
    };
}
