// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EnumDefs.g.h"

class CContentRoot;
struct DragMsg;
struct IInspectable;
class CDependencyObject;
class CEventArgs;
class CDragDropState;

namespace ContentRootInput
{
    class DragDropProcessor
    {
    public:
        DragDropProcessor(_In_ CContentRoot& contentRoot);

        // Raise drag enter, over, leave, and drop events based on a drag message. If bRequireMouseMoveForDragOver is TRUE,
        // and the message's drag position differs from m_xMousePosLast and m_yMousePosLast, a DragOver event is automatically
        // raised and m_{x,y}MousePosLast are updated to the message's drag position if the drop does not encounter an error.
        // Setting bRequireMouseMoveForDragOver to FALSE causes the DragOver message to always fire if there is an element under
        // the drag position. Additionally, m_{x,y}MousePosLast are not updated. If bRaiseSync is TRUE, all events are raised synchronously.
        // Otherwise, events are raised asynchronously.
        _Check_return_ HRESULT ProcessDragDrop(
            _In_ DragMsg *pMsg,
            _In_ bool bRequireMouseMoveForDragOver,
            _In_ bool bRaiseSync,
            _Out_ bool *handled);

        // Raise drag enter, over, leave, and drop events based on a drag message.
        _Check_return_ HRESULT ProcessWinRtDragDrop(
            _In_ DragMsg* message,
            _In_ IInspectable* winRtDragInfo,
            _In_opt_ IInspectable* dragDropAsyncOperation,
            _Inout_opt_ DirectUI::DataPackageOperation* acceptedOperation,
            _In_opt_ CDependencyObject* hitTestRoot);

        xref_ptr<CDependencyObject>& GetDragEnterDONoRef() { return m_dragEnterDO; }

    private:
        _Check_return_ HRESULT ProcessDragEnterLeave(
            _In_ CDependencyObject *pNewElement,
            _In_opt_ CEventArgs *pArgs,
            _In_ XINT32 bSkipLeave,
            _In_ XPOINTF xp,
            _In_ bool bRaiseSync);

        CContentRoot& m_contentRoot;

        xref_ptr<CDependencyObject> m_dragEnterDO;

        // Last mouse location, this is relevant to throttle
        // mouse move messages in windowed controls
        XINT32 m_xMousePosLast = 0;
        XINT32 m_yMousePosLast = 0;

        std::unique_ptr<CDragDropState> m_dragDropState;
    };
};