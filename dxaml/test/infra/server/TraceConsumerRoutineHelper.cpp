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
        std::mutex TraceConsumerRoutineHelper::s_eventMutex;
        TraceEventScope TraceConsumerRoutineHelper::s_eventScope;

        // State shared with the task that runs ProcessTrace. The task keeps its own reference, so a task that
        // outlives its session can't affect the next session.
        struct TraceConsumerRoutineHelper::ProcessTraceTask
        {
            Event returned{ EventOptions::ManualReset, L"TraceConsumerProcessTraceReturned" };
            HRESULT result = S_OK;
        };
        std::shared_ptr<TraceConsumerRoutineHelper::ProcessTraceTask> TraceConsumerRoutineHelper::s_processTraceTask;

        // Set by the event callback when it receives a processing check event
        Event TraceConsumerRoutineHelper::s_processingConfirmedEvent(L"TraceConsumerProcessingConfirmed");

        // Private provider that Start writes check events to, to confirm that ProcessTrace is delivering events
        // {F8160829-0C71-4D52-B555-B6D94CB7D089}
        const GUID TraceConsumerRoutineHelper::s_processingCheckProviderGuid = { 0xf8160829, 0x0c71, 0x4d52, { 0xb5, 0x55, 0xb6, 0xd9, 0x4c, 0xb7, 0xd0, 0x89 } };
        REGHANDLE TraceConsumerRoutineHelper::s_processingCheckProvider = 0;
        void TraceConsumerRoutineHelper::EnableTracingByEventId(int eventId)
        {
            std::lock_guard<std::mutex> lock(s_eventMutex);
            Throw::If((s_IdCountMap.find(eventId) != s_IdCountMap.end()), E_INVALIDARG,
                L"The event you are trying to enable has already been enabled!");
            s_IdCountMap.insert(std::pair<int, unsigned int>(eventId, 0));

        }

        void TraceConsumerRoutineHelper::BeginCountingForProcess(unsigned int processId, __int64 startTimestamp)
        {
            Throw::IfFalse(s_processing, E_FAIL, L"Start tracing before beginning a counting scope.");
            Throw::If(processId == 0 || startTimestamp <= 0, E_INVALIDARG, L"A counting scope requires a process and QPC timestamp.");

            std::lock_guard<std::mutex> lock(s_eventMutex);
            s_eventScope = {processId, startTimestamp};
            s_events.clear();
            for (auto& entry : s_IdCountMap)
            {
                entry.second = 0;
            }
            LOG_OUTPUT(L"TraceConsumer counting scope: pid=%u startQpc=%I64d", processId, startTimestamp);
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

            try
            {
                StartProcessing();
            }
            catch (...)
            {
                // Undo the partial start, so later tests in this server can still trace.
                StopProcessing();
                s_processing = false;
                s_guidProvided = false;
                throw;
            }
        }

        void TraceConsumerRoutineHelper::StartProcessing()
        {
            // ensure s_events is cleared before starting.
            {
                std::lock_guard<std::mutex> lock(s_eventMutex);
                s_eventScope = {};
                s_events.clear();
                s_IdCountMap.clear();
            }
            s_processingConfirmedEvent.Reset();

            if (s_processingCheckProvider == 0)
            {
                LogThrow_IfFailed(HRESULT_FROM_WIN32(EventRegister(&s_processingCheckProviderGuid, nullptr, nullptr, &s_processingCheckProvider)));
            }

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
            LogThrow_IfFailed(s_consumer->EnableProvider(s_processingCheckProviderGuid));

            s_consumer->SetEventRecordCallback(
                [](PEVENT_RECORD pevent_record)
            {
                auto header = pevent_record->EventHeader;

                if (header.ProviderId == s_xamlProviderGuid)
                {
                    DecodeEtwEvents(pevent_record);
                }
                else if (header.ProviderId == s_processingCheckProviderGuid)
                {
                    s_processingConfirmedEvent.Set();
                }
            },
                nullptr);

            LogThrow_IfFailed(s_consumer->OpenTrace());

            // create task to process the event traces. ProcessTrace will block until CloseTrace is called. The task
            // must not throw, because an unobserved task exception takes down the test host. It only uses the
            // consumer and the state it was created with, and records ProcessTrace's result for Stop to report.
            auto consumer = s_consumer.get();
            auto processTraceTask = std::make_shared<ProcessTraceTask>();
            create_task([consumer, processTraceTask]
            {
                processTraceTask->result = consumer->ProcessTrace();
                processTraceTask->returned.Set();
            });
            s_processTraceTask = processTraceTask;

            WaitUntilProcessingEvents();
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
            bool eventListMissing = false;
            {
                std::lock_guard<std::mutex> lock(s_eventMutex);
                eventListMissing = s_IdCountMap.size() == 0 && s_guidProvided;
            }
            // todo: [investigate] right now close trace needs to be called before s_consumer->SetEventRecordCallback is raised.
            //       this does not seem right. i expected s_consumer->SetEventRecordCallback to be raised as events are fired
            // Wait for processing to finish

            // reset processing flag
            s_processing = false;
            s_guidProvided = false;

            // Finish tearing the session down before reporting any failure, so the next Start works even when this
            // Stop fails.
            const HRESULT stopResult = StopProcessing();
            Throw::If(eventListMissing, E_FAIL,
                L"If not using telemetry provider, must call TraceConsumer::AddEventToList to add events to test before stopping trace");
            LogThrow_IfFailed(stopResult);
        }

        // Returns once ProcessTrace is delivering events. Waiting for the task to start isn't enough: it can be paused
        // before it enters ProcessTrace, and a short test's Stop then closes the trace first, so ProcessTrace fails on
        // the closed handle with ERROR_INVALID_HANDLE. Instead, keep writing check events from a private provider
        // enabled on the session until the event callback, which runs inside ProcessTrace, receives one.
        void TraceConsumerRoutineHelper::WaitUntilProcessingEvents()
        {
            const auto deadline = std::chrono::steady_clock::now() + s_processingConfirmedEvent.Timeout;
            while (true)
            {
                EventWriteString(s_processingCheckProvider, 0, 0, L"TraceConsumer processing check");
                s_consumer->Flush();
                if (s_processingConfirmedEvent.WaitForNoThrow(std::chrono::milliseconds(100)))
                {
                    return;
                }

                if (s_processTraceTask->returned.WaitForNoThrow(std::chrono::milliseconds(0)))
                {
                    LogThrow_IfFailed(s_processTraceTask->result);
                    Throw::Exception(E_UNEXPECTED, L"ProcessTrace returned before it delivered any events");
                }

                Throw::If(std::chrono::steady_clock::now() >= deadline, HRESULT_FROM_WIN32(WAIT_TIMEOUT),
                    L"ProcessTrace didn't start delivering events in time");
            }
        }

        // Closes the trace, waits for the processing task and disables the providers. It doesn't throw, so Stop and a
        // failed Start always finish tearing the session down, and it returns the first failure for them to report.
        HRESULT TraceConsumerRoutineHelper::StopProcessing()
        {
            if (!s_consumer)
            {
                return S_OK;
            }

            HRESULT result = S_OK;
            bool taskSettled = true;
            if (s_processTraceTask)
            {
                // CloseTrace makes ProcessTrace return
                result = s_consumer->CloseTrace();
                taskSettled = s_processTraceTask->returned.WaitForNoThrow();
                if (SUCCEEDED(result))
                {
                    result = taskSettled ? s_processTraceTask->result : HRESULT_FROM_WIN32(WAIT_TIMEOUT);
                }
                s_processTraceTask.reset();
            }

            // disable provide and clear callback
            s_consumer->DisableProvider(s_xamlProviderGuid);
            s_consumer->DisableProvider(s_processingCheckProviderGuid);

            if (!taskSettled)
            {
                // The task can still use the consumer, so leak the consumer rather than free it under the task.
                LOG_OUTPUT(L"ProcessTrace didn't return after CloseTrace, so its consumer is leaked rather than freed.");
                (void)s_consumer.release();
            }

            return result;
        }

        void TraceConsumerRoutineHelper::VerifyEventTraced(int eventId, unsigned int count)
        {
            Throw::If(s_processing, E_FAIL, L"TraceConsumer::Stop needs to be called before TraceConsumerRoutineHelper::VerifyEventTraced can be called");
            std::lock_guard<std::mutex> lock(s_eventMutex);

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
            std::lock_guard<std::mutex> lock(s_eventMutex);

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
            std::lock_guard<std::mutex> lock(s_eventMutex);

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
            std::lock_guard<std::mutex> lock(s_eventMutex);
            // ETW delivers buffered records after subscription; filter by emission time, not delivery time.
            if (!s_eventScope.Includes(pevent_record->EventHeader.ProcessId, pevent_record->EventHeader.TimeStamp.QuadPart))
            {
                return;
            }

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