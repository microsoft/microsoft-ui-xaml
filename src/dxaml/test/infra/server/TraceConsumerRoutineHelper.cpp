// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TraceConsumerRoutineHelper.h"
#include <wexassert.h>
#include <cwctype>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Etw::Processor;
using namespace Concurrency;

namespace 
{
    bool IsTraceLoggingEvent(_In_ CONST EVENT_RECORD* EventRecord)
    {
        if (EventRecord->EventHeader.EventDescriptor.Channel == 0xb) 
        {
            return true;
        }

        for (USHORT i = 0; i < EventRecord->ExtendedDataCount; ++i) 
        {
            if (EventRecord->ExtendedData[i].ExtType == EVENT_HEADER_EXT_TYPE_EVENT_SCHEMA_TL) 
            {
                return true;
            }
        }

        return false;
    }
}


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        // Trace logging provider GUID
        GUID TraceConsumerRoutineHelper::s_xamlProviderGuid = TELEMETRY_GUID;

        // cache of events
        std::vector<TraceLoggingEvent> TraceConsumerRoutineHelper::s_events;

        // etw realtime consumer used to consume all traces
        std::unique_ptr<Etw::Processor::EtwRealtimeConsumer> TraceConsumerRoutineHelper::s_consumer;

        // counter to keep the tracename unique when running tests in stress mode
        unsigned int TraceConsumerRoutineHelper::s_traceCounter = 0;

        // flag used to ensure methods are called when expected
        bool TraceConsumerRoutineHelper::s_processing = false;
        bool TraceConsumerRoutineHelper::s_guidProvided = false;
        // EventId map for keeping count of non-telementry events
        std::map<int, unsigned int> TraceConsumerRoutineHelper::s_IdCountMap;

        // Process thread sync event
        Event TraceConsumerRoutineHelper::s_syncEvent(L"TraceConsumerRoutineSync");
        void TraceConsumerRoutineHelper::EnableTracingByEventId(int eventId)
        {
            Throw::If((s_IdCountMap.find(eventId) != s_IdCountMap.end()), E_INVALIDARG,
                L"The event you are trying to enable has already been enabled!");
            s_IdCountMap.insert(std::pair<int, unsigned int>(eventId, 0));

        }
        void TraceConsumerRoutineHelper::Start()
        {
            Throw::If(s_processing, E_FAIL, L"TraceConsumer::Start has already been called");

            // need flag for when different tests use different guid
            if (!s_guidProvided)
            {
                s_xamlProviderGuid = TELEMETRY_GUID;
            }
            // set processing flag
            s_processing = true;
            // ensure s_events is cleared before starting.
            s_events.clear();
            s_IdCountMap.clear();
            s_syncEvent.Reset();

            ++s_traceCounter;
            wchar_t traceName[100];
            swprintf(traceName, L"XamlTestTrace%u", s_traceCounter);
            s_consumer.reset(new EtwRealtimeConsumer(traceName));

            {
                HRESULT enableResult = S_OK;
                int retryCount = 10;
                while (retryCount > 0)
                {
                    enableResult = s_consumer->EnableProvider(s_xamlProviderGuid);
                    if (HRESULT_FROM_WIN32(WAIT_TIMEOUT) == enableResult)
                    {
                        --retryCount;
                        LOG_OUTPUT(L"EtwRealtimeConsumer::EnableProvider failed with timeout.  Waiting 5s and then retrying...");
                        ::Sleep(5000);
                    }
                    else
                    {
                        break;
                    }
                }
                LogThrow_IfFailed(enableResult);
            }

            s_consumer->SetEventRecordCallback(
                [](PEVENT_RECORD pevent_record)
            {
                auto header = pevent_record->EventHeader;

                if (header.ProviderId == s_xamlProviderGuid)
                {
                    DecodeEtwEvents(pevent_record);
                }
            },
                nullptr);

            LogThrow_IfFailed(s_consumer->OpenTrace());

            // create task to process the event traces. ProcessTrace will block until CloseTrace is called
            create_task([]
            {
                LogThrow_IfFailed(s_consumer->ProcessTrace());
                s_syncEvent.Set();
            });
            // need to unregister event on close
        }

        void TraceConsumerRoutineHelper::Start(GUID xamlGuid)
        {
            s_xamlProviderGuid = xamlGuid;
            s_guidProvided = true;
            TraceConsumerRoutineHelper::Start();
        }

        void TraceConsumerRoutineHelper::Stop()
        {
            Throw::IfFalse(s_processing, E_FAIL, L"TraceConsumer::Start was never called");
            Throw::If(s_IdCountMap.size() == 0 && s_guidProvided, E_FAIL,
                L"If not using telemetry provider, must call TraceConsumer::AddEventToList to add events to test before stopping trace");
            // todo: [investigate] right now close trace needs to be called before s_consumer->SetEventRecordCallback is raised.
            //       this does not seem right. i expected s_consumer->SetEventRecordCallback to be raised as events are fired
            // Wait for processing to finish

            // reset processing flag
            s_processing = false;
            s_guidProvided = false;

            LogThrow_IfFailed(s_consumer->CloseTrace());
            s_syncEvent.WaitForDefault();
            // disable provide and clear callback
            s_consumer->DisableProvider(s_xamlProviderGuid);
        }

        void TraceConsumerRoutineHelper::VerifyEventTraced(int eventId, unsigned int count)
        {
            Throw::If(s_processing, E_FAIL, L"TraceConsumer::Stop needs to be called before TraceConsumerRoutineHelper::VerifyEventTraced can be called");

            if (s_IdCountMap[eventId] != count)
            {
                LOG_OUTPUT(L"event %d was not traced as expected", eventId);
                LOG_OUTPUT(L"  expected : %d", count);
                LOG_OUTPUT(L"    actual : %d", s_IdCountMap[eventId]);
                VERIFY_FAIL();
            }
        }
        void TraceConsumerRoutineHelper::VerifyEventTraced(int eventId)
        {
            Throw::If(s_processing, E_FAIL, L"TraceConsumer::Stop needs to be called before TraceConsumerRoutineHelper::VerifyEventTraced can be called");

            if (s_IdCountMap[eventId] == 0)
            {
                LOG_OUTPUT(L"event %d was not traced at all", eventId);
                VERIFY_FAIL();
            }
            else
            {
                LOG_OUTPUT(L"event %d was traced %d times", eventId, s_IdCountMap[eventId]);
            }
        }
        void TraceConsumerRoutineHelper::VerifyEventTraced(const wchar_t* eventName, unsigned int count)
        {
            Throw::If(s_processing, E_FAIL, L"TraceConsumer::Stop needs to be called before TraceConsumerRoutineHelper::VerifyEventTraced can be called");

            LOG_OUTPUT(L"verify event '%s' was traced '%d' time(s)", eventName, count);

            // for each event do case-insensitive compare of name to eventName
            auto matchCount = std::count_if(s_events.begin(), s_events.end(), [eventName](const TraceLoggingEvent& e)
            {
                return std::equal(e.name.begin(), e.name.end(), eventName, [](const wchar_t lhs, const wchar_t rhs)
                {
                    return std::towlower(lhs) == std::towlower(rhs);
                });
            });


            if (static_cast<unsigned int>(matchCount) != count)
            {
                LOG_OUTPUT(L"event '%s' was not traced as expected", eventName);
                LOG_OUTPUT(L"  expected : %d", count);
                LOG_OUTPUT(L"    actual : %I64d", matchCount);

                if (s_events.size() == 0)
                {
                    LOG_OUTPUT(L"\nno event captured");
                }
                else
                {
                    LOG_OUTPUT(L"\nall captured events");

                    for (auto& event : s_events)
                    {
                        // Using c_str() because LOG_OUTPUT inconsistently throws exception when std::wstring passed in
                        LOG_OUTPUT(L"      event name : %s", event.name.c_str());
                        LOG_OUTPUT(L"        event id : %d", event.id);
                        LOG_OUTPUT(L"     event level : %d", event.level);
                        LOG_OUTPUT(L"    event opcode : %d", event.opcode);
                        LOG_OUTPUT(L"  event provider : %s\n", event.provider.c_str());
                    }
                }

                VERIFY_FAIL();
            }
        }

        // decode tracelogging event data from event record.
        // todo: need to update when better support for telemetry data is added
        //       currently you can see the part c data field names, but work needs to be done to properly decode values
        void TraceConsumerRoutineHelper::DecodeEtwEvents(const PEVENT_RECORD pevent_record)
        {
            std::unique_ptr<TRACE_EVENT_INFO> traceEventInfo;
            unsigned long dataSize = 0;
            // Pass in null for buffer to get the required capacity
            DWORD status = TdhGetEventInformation(pevent_record, 0, nullptr, nullptr, &dataSize);
            if (ERROR_INSUFFICIENT_BUFFER == status)
            {
                traceEventInfo.reset(reinterpret_cast<TRACE_EVENT_INFO*>(new BYTE[dataSize]));
                status = TdhGetEventInformation(pevent_record, 0, nullptr, traceEventInfo.get(), &dataSize);
                if (status == ERROR_SUCCESS)
                {
                    if (IsTraceLoggingEvent(pevent_record))
                    {
                        std::wstring name;
                        std::wstring provider;
                        int id = 0;
                        int opcode = 0;
                        int level = 0;
                        if (traceEventInfo->TaskNameOffset > 0)
                        {
                            name = reinterpret_cast<LPWSTR>(reinterpret_cast<PBYTE>(traceEventInfo.get())+traceEventInfo->TaskNameOffset);
                        }

                        if (traceEventInfo->ProviderNameOffset > 0)
                        {
                            provider = reinterpret_cast<LPWSTR>(reinterpret_cast<PBYTE>(traceEventInfo.get())+traceEventInfo->ProviderNameOffset);
                        }

                        id = static_cast<int>(traceEventInfo->EventDescriptor.Id);
                        level = static_cast<int>(traceEventInfo->EventDescriptor.Level);
                        opcode = static_cast<int>(traceEventInfo->EventDescriptor.Opcode);
                        s_events.emplace_back(name, provider, id, opcode, level);
                    }
                    else
                    {
                        if (s_IdCountMap.find(traceEventInfo->EventDescriptor.Id) != s_IdCountMap.end())
                        {
                            ++s_IdCountMap.at(traceEventInfo->EventDescriptor.Id);
                        }
                    }

                    return;
                }
            }

            LOG_OUTPUT(L"Error: %x occurred trying to get size of event data", status);
            VERIFY_FAIL();
        }
    }
} } } }