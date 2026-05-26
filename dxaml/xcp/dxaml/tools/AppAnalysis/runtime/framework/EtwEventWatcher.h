// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <map>
#include "helpers.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    class EtwEventWatcher
        : public wrl::RuntimeClass<appanalysis::IEtwEventWatcher, appanalysis::IEtwEventWatcherPrivate, wrl::FtmBase>
    {
        InspectableClass(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwEventWatcher, BaseTrust);

    public:

        EtwEventWatcher()
            : m_hasStarted(false)
        {
        }

        virtual ~EtwEventWatcher();

        HRESULT RuntimeClassInitialize();

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWacher::RegisterEvent
        //
        IFACEMETHOD(RegisterEvent)(
            _In_ appanalysis::IEtwEvent* etwEvent,
            _In_ appanalysis::IEtwEventRecordCallback* callback
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWacher::Start
        //
        IFACEMETHOD(Start)(
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWacher::Stop
        //
        IFACEMETHOD(Stop)(
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwEventWacher::get_RegisteredEvents
        //
        IFACEMETHOD(get_RegisteredEvents)(
            _COM_Outptr_ wfc::IVectorView<appanalysis::EtwEvent*>** registeredEvents
            ) override;


    private:

        using EventVector = wfci_::Vector<appanalysis::EtwEvent*>;
        wrl::ComPtr<EventVector> m_registeredEvents;

        using CallbackRegistrationMap = wfci_::HashMap<appanalysis::EtwEvent*, appanalysis::IEtwEventRecordCallback*>;
        wrl::ComPtr<CallbackRegistrationMap> m_callbackRegistrationCache;

        bool m_hasStarted;
    };

} } }
