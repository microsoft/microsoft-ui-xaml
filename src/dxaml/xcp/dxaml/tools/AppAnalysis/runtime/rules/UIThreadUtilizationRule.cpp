// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"

#define CSwitch_value 36
#define CSwitch_version 2

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
// The UIThreadUtilization rule detects (to the best that we can) of when the UI thread is not being
// run during critical start up paths. We are defining startup here to be when we finish the first 
// frame. This of course may not be what the app considers to be startup, but we have no way of 
// detecting that for every single app.
class UIThreadUtilization
    : public EtwRuleImpl<UIThreadUtilization, appanalysis::RuleCategories_Performance>
{

public:

    UIThreadUtilization()
        : m_uiThreadId(0)
        , m_putSourceTimeStamp(0)
        , m_lastSwitchOut(0)
        , m_inactiveTime(0)
        , m_firstFrameFired(false)
    {
    }
    
    virtual ~UIThreadUtilization()
    {
    }

    // When processing the context switch event, we keep track of the time the thread was 
    // switched out. This is how xperf calculates the UI thread utilization.
    HRESULT UIThreadUtilization::ProcessContextSwitchEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        // Only care if we have already had InitializeCore called where
        // we know the thread ID for the UI thread.
        if (m_uiThreadId > 0 && m_putSourceTimeStamp > 0 && !m_firstFrameFired)
        {
            UINT32 oldThread = 0;
            IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"OldThreadId"), &oldThread));
            // Check if we are switching off the UI thread and cache the
            // timestamp.
            if (oldThread == m_uiThreadId)
            {
                IFC_RETURN(pEvent->get_Timestamp(&m_lastSwitchOut));
            }
            else
            {
                // If not switching off UI thread, check to see if we are switching 
                // to the ui thread. If we are, then we subtract the cached switch 
                // out timestamp from the current timestamp to get the time
                // we were off the UI thread and add it to our running count.
                UINT32 newThread = 0;
                IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"NewThreadId"), &newThread));

                // If we haven't gotten a switch out yet, then we'll have to ignore this.
                // During realtime scenarios we can't always trust the sequents of these
                // events if coming from different threads.
                if (newThread == m_uiThreadId && m_lastSwitchOut != 0)
                {
                    LONGLONG timeStamp = 0;
                    IFC_RETURN(pEvent->get_Timestamp(&timeStamp));
                    m_inactiveTime += (timeStamp - m_lastSwitchOut);
                    m_lastSwitchOut = 0;
                }
            }
        }
        return S_OK;
    }

    HRESULT UIThreadUtilization::ProcessPutSourceStartEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        IFC_RETURN(pEvent->get_ThreadId(&m_uiThreadId));
        IFC_RETURN(pEvent->get_Timestamp(&m_putSourceTimeStamp));

        return S_OK;
    }

    // When we reach the first frame end event, we take the total time
    // (current time stamp - put source time stamp) and calculate the 
    // percentage based off the total inactive time of the UI thread.
    HRESULT UIThreadUtilization::ProcessFrameEndEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        if (m_putSourceTimeStamp > 0 && !m_firstFrameFired)
        {
            m_firstFrameFired = true;

            LONGLONG frameSubmitTimeStamp = 0;
            IFC_RETURN(pEvent->get_Timestamp(&frameSubmitTimeStamp));

            LONGLONG totalTime = frameSubmitTimeStamp - m_putSourceTimeStamp;
            double percentActive = 100 * ((totalTime - m_inactiveTime) / static_cast<double>(totalTime));
            if (percentActive > 0.0 && percentActive < static_cast<double>(c_minPercentActive))
            {
                RuleTriggeredEventArgs::CreateParams params;
                params.measurement.Unit = appanalysis::MeasurementUnit_Percentage;
                params.measurement.Value = percentActive;

                params.timeline.Start = m_putSourceTimeStamp;
                params.timeline.Stop = frameSubmitTimeStamp;

                wil::shared_hstring minPercent;
                IFC_RETURN(AppAnalysisHelpers::ToString(c_minPercentActive, &minPercent));

                wil::shared_hstring actualPercent;
                IFC_RETURN(AppAnalysisHelpers::ToString(static_cast<unsigned int>(percentActive), &minPercent));

                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, UI_THREAD_UTILIZATION_DESCRIPTION));
                IFC_RETURN(params.description->Append(minPercent.get()));
                IFC_RETURN(params.description->Append(actualPercent.get()));

                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.solution, UI_THREAD_UTILIZATION_SOLUTION));
                wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> notificationInfo;
                IFC_RETURN(CreateRuleTriggeredEventArgs(params, &notificationInfo));

                FireNotification(notificationInfo.Get());
            }
        }

        return S_OK;
    }

private:

    UINT32 m_uiThreadId;
    LONGLONG m_putSourceTimeStamp;
    LONGLONG m_inactiveTime;
    LONGLONG m_lastSwitchOut;
    bool m_firstFrameFired;

    const unsigned int c_minPercentActive = 80;
};

// Copied from Windows DDK (wmiguid.h)
DEFINE_GUID ( /* 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c */
    ThreadGuid,
    0x3d6fa8d1,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

BEGIN_PROVIDERS(UIThreadUtilization)
    DECLARE_KERNEL_PROVIDER(ThreadGuid, EVENT_TRACE_FLAG_CSWITCH)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(UIThreadUtilization)
    DECLARE_EVENT_CALLBACK(ThreadGuid, CSwitch_value, CSwitch_version, &UIThreadUtilization::ProcessContextSwitchEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, PutSourceBegin_value, EventVersion_0, &UIThreadUtilization::ProcessPutSourceStartEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, FrameEnd_value, EventVersion_0, &UIThreadUtilization::ProcessFrameEndEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT UIThreadUtilization_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<UIThreadUtilization> rule;
    IFC_RETURN(UIThreadUtilization::CreateInstance(
        UI_THREAD_UTILIZATION_ID, UI_THREAD_UTILIZATION_TITLE, UI_THREAD_UTILIZATION_IMPACT, 
        UI_THREAD_UTILIZATION_LINK_TITLE, UI_THREAD_UTILIZATION_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));
    
    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}
} } }
