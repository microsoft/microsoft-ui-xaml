// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EtwEventWatcher.h"
#include "helpers.h"
#include "EventProcessor.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
    
        EtwEventWatcher::~EtwEventWatcher()
        {
            Stop();
        }

        HRESULT
        EtwEventWatcher::RuntimeClassInitialize()
        {
            IFC_RETURN(EventVector::Make(&m_registeredEvents));
            IFC_RETURN(CallbackRegistrationMap::Make(&m_callbackRegistrationCache));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWatcher::RegisterEvent
        //
        IFACEMETHODIMP
            EtwEventWatcher::RegisterEvent(
                _In_ appanalysis::IEtwEvent* etwEvent,
                _In_ appanalysis::IEtwEventRecordCallback* callback
                )
        {
            if (m_hasStarted)
            {
                return E_ILLEGAL_STATE_CHANGE;
            }

            boolean replaced = false;
            IFC_RETURN(m_callbackRegistrationCache->Insert(etwEvent, callback, &replaced));
            
            // we shouldn't be having multiple callbacks declared for the same event
            ASSERT(!replaced);

            if (!replaced)
            {
                // only append the event if we haven't registered it yet
                IFC_RETURN(m_registeredEvents->Append(etwEvent));
            }

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWatcher::get_RegisteredEvents
        //
        IFACEMETHODIMP 
        EtwEventWatcher::get_RegisteredEvents(
            _COM_Outptr_ wfc::IVectorView<appanalysis::EtwEvent*>** registeredEvents
            )
        {
            ARG_VALIDRETURNPOINTER(registeredEvents);
            *registeredEvents = nullptr;

            IFC_RETURN(m_registeredEvents->GetView(registeredEvents));
            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWatcher::Start
        //   Start adds all of the events that have been registered with this watcher
        //   to the eventprocessor.
        IFACEMETHODIMP
        EtwEventWatcher::Start()
        {
            if (m_hasStarted)
            {
                return S_OK;
            }
                
            UINT32 eventsRegistered = 0;
            IFC_RETURN(m_registeredEvents->get_Size(&eventsRegistered));
            if (eventsRegistered == 0)
            {
                return E_NOT_VALID_STATE;
            }

            // Enable all the events for the provider that this watcher is interested in
            for (UINT32 i = 0; i < eventsRegistered; i++)
            {
                wrl::ComPtr<appanalysis::IEtwEvent> etwEvent;
                IFC_RETURN(m_registeredEvents->GetAt(i, &etwEvent));

                wrl::ComPtr<appanalysis::IEtwEventRecordCallback> callback;
                IFC_RETURN(m_callbackRegistrationCache->Lookup(etwEvent.Get(), &callback));
                  
                IFC_RETURN(EventProcessor::RegisterEventStatic(etwEvent.Get(), callback.Get()));

            }

            m_hasStarted = true;
            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWatcher::Stop
        //   removes the events this watcher was listening to from the EventProcessor
        IFACEMETHODIMP
        EtwEventWatcher::Stop()
        {
            UINT32 eventsRegistered = 0;
            IFC_RETURN(m_registeredEvents->get_Size(&eventsRegistered));
            // Enable all the events for the provider that this watcher is interested in
            for (UINT32 i = 0; i < eventsRegistered; i++)
            {
                wrl::ComPtr<appanalysis::IEtwEvent> etwEvent;
                IFC_RETURN(m_registeredEvents->GetAt(i, &etwEvent));

                wrl::ComPtr<appanalysis::IEtwEventRecordCallback> callback;
                IFC_RETURN(m_callbackRegistrationCache->Lookup(etwEvent.Get(), &callback));

                IFC_RETURN(EventProcessor::UnregisterEventStatic(etwEvent.Get(), callback.Get()));

            }

            // clear the registration caches now that we've unregistered them all
            IFC_RETURN(m_callbackRegistrationCache->Clear());
            IFC_RETURN(m_registeredEvents->Clear());

            m_hasStarted = false;
            return S_OK;
        }



} } }