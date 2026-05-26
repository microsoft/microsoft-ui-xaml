// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Indexes.g.h>

class CContentRoot;
class CDependencyObject;
class CEventArgs;
class CUIElement;

namespace ContentRootAdapters
{
    class ContentRootEventListener
    {
    public:
        ContentRootEventListener(_In_ CContentRoot& contentRoot);
        ~ContentRootEventListener();

    private:
        _Check_return_ HRESULT RegisterTabProcessEventHandler();
        _Check_return_ HRESULT UnregisterTabProcessEventHandler();
        _Check_return_ HRESULT RegisterContextMenuOpeningEventHandler();
        _Check_return_ HRESULT UnregisterContextMenuOpeningEventHandler();
        _Check_return_ HRESULT RegisterManipulationInertiaProcessingEventHandler();
        _Check_return_ HRESULT UnregisterManipulationInertiaProcessingEventHandler();
        _Check_return_ HRESULT RegisterRightTappedEventHandler();
        _Check_return_ HRESULT UnregisterRightTappedEventHandler();

        _Check_return_ HRESULT static TabProcessingEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
        _Check_return_ HRESULT static ContextMenuOpeningEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
        _Check_return_ HRESULT static RightTappedEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
        void static ManipulationInertiaProcessingEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

        xref_ptr<CUIElement> m_element;

        static const KnownEventIndex textControlEventsTypes[];
    };
};