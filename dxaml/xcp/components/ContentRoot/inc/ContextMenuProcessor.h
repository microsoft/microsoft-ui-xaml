// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CContentRoot;
class CDispatcherTimer;
class CEventArgs;

namespace ContentRootInput
{
    class ContextMenuProcessor
    {
    public:
        ContextMenuProcessor(_In_ CContentRoot& contentRoot);

        _Check_return_ HRESULT RaiseContextRequestedEvent(
            _In_ CDependencyObject* pSource,
            _In_ wf::Point point,
            _In_ bool isTouchInput);

        _Check_return_ HRESULT ProcessContextRequestOnKeyboardInput(
            _In_ CDependencyObject* pSource,
            _In_ wsy::VirtualKey virtualKey,
            _In_ XUINT32 modifierKeys);

        _Check_return_ HRESULT ProcessContextRequestOnHoldingGesture(_In_ CDependencyObject *pElement);

        bool IsContextMenuOnHolding() const
        {
            return m_isContextMenuOnHolding;
        }

        void SetIsContextMenuOnHolding(_In_ bool value)
        {
            m_isContextMenuOnHolding = value;
        }

        xref_ptr<CDispatcherTimer>& GetContextMenuTimer()
        {
            return m_contextMenuTimer;
        }

        void SetContextMenuOnHoldingTouchPoint(_In_ wf::Point& point)
        {
            m_contextMenuOnHoldingTouchPoint = point;
        }

    private:
        // Called when the context menu timer for Press & Hold gesture expires
        static _Check_return_ HRESULT OnContextRequestOnHoldingTimeout(
            _In_ CDependencyObject *pSender,
            _In_ CEventArgs* pEventArgs);

        CContentRoot& m_contentRoot;

        bool m_isContextMenuOnHolding = false;
        xref_ptr<CDispatcherTimer> m_contextMenuTimer;

        wf::Point m_contextMenuOnHoldingTouchPoint = { -1, -1 };
    };
};