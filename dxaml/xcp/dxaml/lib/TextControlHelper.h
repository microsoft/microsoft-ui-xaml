// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Callback.h"

namespace TextControlHelper
{
    template<typename TEditInterface, typename TEditClass>
    _Check_return_ HRESULT OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeObject, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
    {
        handled = false;
        ctl::ComPtr<DependencyObject> peer;
        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pNativeObject, &peer));

        if (peer)
        {
            ctl::ComPtr<TEditInterface> control;
            IFC_RETURN(peer.As(&control));
            IFCPTR_RETURN(control);

            ctl::ComPtr<TEditClass> peerAsClassObject;
            IFC_RETURN(peer.As(&peerAsClassObject));

            typename TEditClass::ContextMenuOpeningEventSourceType* eventSource = nullptr;
            IFC_RETURN(peerAsClassObject->GetContextMenuOpeningEventSourceNoRef(&eventSource));

            ctl::ComPtr<ContextMenuEventArgs> contextMenuOpeningEventArgs;
            IFC_RETURN(ctl::make(&(contextMenuOpeningEventArgs)));

            contextMenuOpeningEventArgs->put_CursorLeft(cursorLeft);
            contextMenuOpeningEventArgs->put_CursorTop(cursorTop);
            contextMenuOpeningEventArgs->put_Handled(FALSE);

            IFC_RETURN(eventSource->Raise(control.Get(), contextMenuOpeningEventArgs.Get()));

            BOOLEAN eventHandled = FALSE;
            IFC_RETURN(contextMenuOpeningEventArgs->get_Handled(&eventHandled));
            handled = !!eventHandled;
        }

        return S_OK;
    }

    template<typename TEditClass>
    _Check_return_ static HRESULT callback(ctl::WeakRefPtr peerWeakRef)
    {
        auto peer = peerWeakRef.AsOrNull<TEditClass>();

        if (peer)
        {
            IFC_RETURN(peer->UpdateSelectionFlyoutVisibility());
        }

        return S_OK;
    }

    template<typename TEditClass>
    _Check_return_ HRESULT QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeObject)
    {
        ctl::ComPtr<DependencyObject> peer;
        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pNativeObject, &peer));

        if (peer)
        {
            ctl::ComPtr<TEditClass> peerAsClassObject;
            IFC_RETURN(peer.As(&peerAsClassObject));

            ctl::WeakRefPtr peerWeakRef;
            IFC_RETURN(peer.AsWeak(&peerWeakRef));

            IFC_RETURN(peerAsClassObject->GetXamlDispatcherNoRef()->RunAsync(
                MakeCallback<ctl::WeakRefPtr,ctl::WeakRefPtr>(
                    &callback<TEditClass>,
                    peerWeakRef)));
        }

        return S_OK;
    }

    template<typename TCoreEditClass>
    _Check_return_ HRESULT UpdateSelectionFlyoutVisibility(_In_ DirectUI::DependencyObject *dxamlObject)
    {
        TCoreEditClass *nativeObject = do_pointer_cast<TCoreEditClass>(dxamlObject->GetHandle());

        if (nativeObject)
        {
            IFC_RETURN(nativeObject->UpdateSelectionFlyoutVisibility());
        }

        return S_OK;
    }
}