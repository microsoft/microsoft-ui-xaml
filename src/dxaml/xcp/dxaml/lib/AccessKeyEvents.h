// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <comInstantiation.h>
#include <JoltClasses.h>

namespace AccessKeys
{
class AKEvents
{
public:
    typedef wf::ITypedEventHandler<IInspectable*, IInspectable*> HandlerType;
    typedef DirectUI::CEventSource<HandlerType, IInspectable, IInspectable> EventSourceType;

    _Check_return_ HRESULT RaiseIsActiveChanged(_In_ IInspectable* sender, _In_ IInspectable* args)
    {
        if (isActiveChangedEventSource != nullptr)
        {
            IFC_RETURN(isActiveChangedEventSource->Raise(sender, args));
        }
        return S_OK;
    }

    _Check_return_ HRESULT add_IsActiveChanged(_In_ HandlerType* handler, _Out_ EventRegistrationToken* token)
    {
        IFC_RETURN(EnsureEventSource());
        IFC_RETURN(isActiveChangedEventSource->AddHandler(handler));
        token->value = (INT64)handler;
        return S_OK;
    }

    _Check_return_ HRESULT remove_IsActiveChanged(_In_ EventRegistrationToken token)
    {
        if (isActiveChangedEventSource == nullptr)
        {
            // No event source means no event handlers have beed added, so removing one is invalid
            return E_FAIL;
        }
        IFC_RETURN(isActiveChangedEventSource->RemoveHandler(reinterpret_cast<HandlerType*>(token.value)));
        
        
        // If leak detection is enabled, then reset the event source if it's empty. This is a special event because
        // it is static. Most event sources are tied to an object and once that goes out of scope, the event source gets
        // destroyed. The only other event similar to this is the render event which get's cleaned up in DXamlCore::ClearCaches.
#if XCP_MONITOR
        if (!isActiveChangedEventSource->HasHandlers())
        {
            isActiveChangedEventSource.Reset();
        }
#endif
        return S_OK;
    }

    bool GetIsDisplayModeEnabledForCurrentThread() const { return m_isDisplayModeEnabledForCurrentThread; }
    void SetIsDisplayModeEnabledForCurrentThread(bool value) { m_isDisplayModeEnabledForCurrentThread = value; }

private:
    _Check_return_ HRESULT EnsureEventSource()
    {
        if (isActiveChangedEventSource)
        {
            return S_OK;
        }

        ctl::ComPtr<EventSourceType> newEventSource;

        IFC_RETURN(ctl::make(&newEventSource));
        newEventSource->Initialize(
            KnownEventIndex::AccessKeyManager_IsDisplayModeEnabledChanged,
            nullptr,
            false /* bCoreEvent */
#if DBG
            , true // This is a static event
#endif
            );

        isActiveChangedEventSource = newEventSource;
        return S_OK;
    }

    ctl::ComPtr<EventSourceType> isActiveChangedEventSource;
    bool m_isDisplayModeEnabledForCurrentThread {false};
};

}

