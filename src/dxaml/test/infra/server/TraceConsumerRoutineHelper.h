// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <vector>
#include <map>
#include <chrono>
#include <TestEvent.h>
#include "etwrealtimeconsumer.h"
#include <TraceLoggingEvent.h>
#include <ppltasks.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {
        const GUID TELEMETRY_GUID = { 0x2dc72f6e, 0xe4d1, 0x5f58, { 0x32, 0x45, 0x09, 0xa4, 0x24, 0x37, 0x99, 0xdd } };
        class TraceConsumerRoutineHelper
        {
        public:
            static void EnableTracingByEventId(int eventId);
            static void Start(GUID xamlGuid);
            static void Start();
            static void Stop();
            static void VerifyEventTraced(const wchar_t* eventName, unsigned int count);
            static void VerifyEventTraced(int eventId, unsigned int count);
            static void VerifyEventTraced(int eventId);

        private:
            static void DecodeEtwEvents(const PEVENT_RECORD pevent_record);
            static GUID s_xamlProviderGuid;
            static std::unique_ptr<Etw::Processor::EtwRealtimeConsumer> s_consumer;
            static unsigned int s_traceCounter;
            static std::vector<TraceLoggingEvent> s_events;
            static bool s_processing;
            static bool s_guidProvided;
            static std::map<int, unsigned int> s_IdCountMap;
            static Event s_syncEvent;
        };
    }
} } } }