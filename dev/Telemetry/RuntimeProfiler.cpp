// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include "RuntimeProfiler.h"
#include "TraceLogging.h"

#pragma warning(push)
#pragma warning(disable : 26812)

#define DEFINE_PROFILEGROUP(name, group, size) \
    CMethodProfileGroup<size>        name(group)

// defined in dllmain.cpp with values from version.h
extern const char *g_BinaryVersion;

namespace RuntimeProfiler {

    void UninitializeRuntimeProfiler();

    struct FunctionTelemetryCount
    {
        volatile LONG      *pInstanceCount{ nullptr };
        volatile UINT16     uTypeIndex{};
        volatile UINT16     uMethodIndex{};
    };

    class CMethodProfileGroupBase
    {
    public:
        virtual void RegisterMethod(UINT16 uTypeIndex, UINT16 uMethodIndex, volatile LONG *pCount) noexcept = 0;
        virtual void FireEvent(bool bSuspend) noexcept = 0;
    };

    template <size_t size>
    class CMethodProfileGroup: public CMethodProfileGroupBase
    {
    public:
        static const int TableSize = size;
        
        CMethodProfileGroup(ProfileGroup group)
        :   m_cMethods(0)
        ,   m_group(group)
        {
            //  Note: We are declaring these objects in a global scope which
            //      basically means that anything we call here in the
            //      constructor needs to be safe to call during DllMain.
        }
        
        ~CMethodProfileGroup()
        {
            //  Ditto from above.
            UninitializeRuntimeProfiler();
        }
        
        void RegisterMethod(UINT16 uTypeIndex, UINT16 uMethodIndex, volatile LONG *pCount) noexcept override
        {
            static_assert(sizeof(LONG) == sizeof(UINT32), "Since we're using InterlockedIncrement, make sure that this is the same size independent of build flavors.");
            
            //  Zero-based index
            const LONG WriteIndex = ::InterlockedIncrement(&m_cMethods) - 1;
            
            if (WriteIndex < (LONG)(m_Counts.max_size()))
            {
                m_Counts[WriteIndex].uTypeIndex          = uTypeIndex;
                m_Counts[WriteIndex].uMethodIndex        = uMethodIndex;
                
                //  Note:  This pointer is the last thing to be set, this is
                //    intentional, FireEvent will check the this pointer and
                //    if set will assume that the rest of this structure is
                //    valid, do not change the order.
                m_Counts[WriteIndex].pInstanceCount      = pCount;
                
                //  FireEvent() will reset counts to zero and we don't want
                //  RegisterMethod() to be called again, thus we set the
                //  initial static value to -1, to be incremented to 0 on first
                //  call and we increment again for an accurate count.
                ::InterlockedIncrement(pCount);
            }
        }
        
        void FireEvent(bool bSuspend) noexcept override
        {
            if (!g_IsTelemetryProviderEnabled)
            {
                // Trace logging provider for Microsoft telemetry is not enabled. Exit right away.
                return;
            }

            const UINT32      ArraySize = (UINT32)(m_Counts.max_size());
            const bool        bOverflow = ((UINT32)(m_cMethods) >= ArraySize);
            UINT32      cMethods = (UINT32)m_cMethods;
            bool        bStringOverflow = false;
            UINT16      cMethodsLogged = 0;
            bool        bSeparator = false;
            
            //  Each entry will look like this:
            //  [XX|YYYY]:ZZZ
            //  Conservatively accounting for 20 characters per entry depending
            //  on the length of the numbers.
            WCHAR       OutputBuffer[20 * TableSize];
            size_t      cchDest = ARRAYSIZE(OutputBuffer);
   
            // min
            cMethods = std::min(cMethods, ArraySize);
   
            PWSTR       pszDest = &(OutputBuffer[0]);
            
            for (UINT32 ii = 0; ii < cMethods; ii++)
            {
                LONG        cHits;

                if (!m_Counts[ii].pInstanceCount)
                {
                    //  In the middle of RegisterMethod on another thread,
                    //  we'll forgo logging this method for now and pick it
                    //  up on the next FireEvent().
                    continue;
                }
            
                //  Zeroing out AND getting value.
                cHits = InterlockedExchange(m_Counts[ii].pInstanceCount, 0);
            
                if (0 != cHits)
                {
                    //  We're using id's instead.  The entry in the list will
                    //  look like '[type index|method index]:count'
                    const HRESULT hr = StringCchPrintfExW(
                            pszDest,
                            cchDest,
                            &pszDest,
                            &cchDest,
                            STRSAFE_NULL_ON_FAILURE,
                            L"%ls[%d|%d]:%d",
                            (bSeparator?L",":L""),
                            (int)m_Counts[ii].uTypeIndex,
                            (int)m_Counts[ii].uMethodIndex,
                            cHits);

                    if (S_OK == hr)
                    {
                        cMethodsLogged++;
                        bSeparator = true;
                    }
                    else if ((STRSAFE_E_INSUFFICIENT_BUFFER == hr) ||
                             (STRSAFE_E_INVALID_PARAMETER == hr))
                    {
                        //  The only legit ways to get invalid parameter
                        //    here is ccDest == 0, so it's effectively an
                        //    overflow condition.  
                        bStringOverflow = true;
                        break;
                    }
                }
            }
        
            if (0 == cMethodsLogged)
            {
                return;
            }
            
            TraceLoggingWrite(  
                g_hTelemetryProvider,  
                "RuntimeProfiler",
                TraceLoggingDescription("XAML methods that have been called."),
                TraceLoggingString(g_BinaryVersion, "BinaryVersion"),
                TraceLoggingWideString(OutputBuffer, "ApiCounts"),
                TraceLoggingUInt32(((UINT32)m_group), "ProfileGroupId"),
                TraceLoggingUInt32(((UINT32)cMethods),"TotalCount"),
                TraceLoggingBoolean(bSuspend, "OnSuspend"),
                TraceLoggingBoolean(bOverflow, "Overflow"),
                TraceLoggingBoolean(bStringOverflow, "StringOverflow"),
                TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }
        
    private:
        std::array<FunctionTelemetryCount, TableSize>   m_Counts = {0};
        LONG                                            m_cMethods;
        ProfileGroup                                    m_group;
    };  //  class CMethodProfileGroup


    //  Yes, we're declaring this as a global, the ctor/dtor are implemented
    //  very carefully and this will not create issues with DllMain().
    DEFINE_PROFILEGROUP(gGroupClasses, PG_Class, ProfId_Size + ProfMemberId_Size);

    struct ProfileGroupInfo
    {
        CMethodProfileGroupBase    *pGroup;
        const char                 *pszGroupName;
    } gProfileGroups[] = 
    {
        { static_cast<CMethodProfileGroupBase*>(&gGroupClasses), "Classes" },
    };

    using namespace std::chrono;
    constexpr auto  EventFrequency = 20min;

    void FireEvent(bool bSuspend) noexcept
    {
        for (auto group : gProfileGroups)
        {
            group.pGroup->FireEvent(bSuspend);
        }
    }

    VOID CALLBACK TPTimerCallback(PTP_CALLBACK_INSTANCE, PVOID, PTP_TIMER) noexcept
    {
        FireEvent(false);
    }

    PTP_TIMER   g_pTimer = nullptr;

    BOOL CALLBACK CancelTimer(PINIT_ONCE /* InitOnce */, PVOID /* Parameter */, PVOID* /* context */)
    {
        if (nullptr != g_pTimer)
        {
            //  Canceling timer.
            
            //  Note:  We're called on a global destructor, so we are not
            //    calling WaitForThreadpoolTimerCallbacks() to prevent
            //    deadlocks.
            SetThreadpoolTimer(g_pTimer, nullptr, 0, 0);
            CloseThreadpoolTimer(g_pTimer);
            
            
            g_pTimer = nullptr;
        }
        
        //  Either way, no active timer.
        return (TRUE);
    }

    void UninitializeRuntimeProfiler()
    {
        static INIT_ONCE    UninitProfiler = INIT_ONCE_STATIC_INIT;
        
        InitOnceExecuteOnce(&UninitProfiler, CancelTimer, nullptr, nullptr);
    }

    BOOL CALLBACK InitializeRuntimeProfiler(PINIT_ONCE /* InitOnce */, PVOID /* Parameter */, PVOID* /* context */)
    {
        g_pTimer = ::CreateThreadpoolTimer(TPTimerCallback, nullptr, nullptr);
    
        if (nullptr != g_pTimer)
        {
            LARGE_INTEGER   lidueTime;
            FILETIME        ftdueTime;

            //  Setting periodic timer.  Using negative time in 100 nanosecond
            //  intervals to indicate relative time.
            lidueTime.QuadPart = -10000 * (LONGLONG)(std::chrono::milliseconds(EventFrequency).count());
        
            ftdueTime.dwHighDateTime = (DWORD)(lidueTime.HighPart);
            ftdueTime.dwLowDateTime  = lidueTime.LowPart;
            
            //  Setting the callback window length to 60 seconds since the
            //  timing of the event is not critical
            SetThreadpoolTimer(g_pTimer, &ftdueTime, (DWORD)(std::chrono::milliseconds(EventFrequency).count()), 60 * 1000);
        }
        
        //  Since MUX doesn't piggyback the WUX Extension suspend handler,
        //  we sign up for suspension notifications.
        try
        {
            winrt::Application::Current().Suspending(([](auto&, auto&)
                {
                    FireEvent(true);
                }
            ));
        }
        catch (winrt::hresult_error e)
        {
            // We might not have an Application instance object in XamlPresenter scenarios
            // because we don't need it.
        }

        return ((nullptr != g_pTimer)?TRUE:FALSE);
    }

    void RegisterMethod(ProfileGroup group, UINT16 uTypeIndex, UINT16 uMethodIndex, volatile LONG *pCount) noexcept
    {
        static INIT_ONCE            InitProfiler = INIT_ONCE_STATIC_INIT;
        CMethodProfileGroupBase    *pGroup = gProfileGroups[(int)group].pGroup;
    
        InitOnceExecuteOnce(&InitProfiler, InitializeRuntimeProfiler, nullptr, nullptr);

        return (pGroup->RegisterMethod(uTypeIndex, uMethodIndex, pCount));
    }

} // namespace RuntimeProfiler

//  This will be exported by WUX Extension library
STDAPI_(void) SendTelemetryOnSuspend() noexcept
{
    RuntimeProfiler::FireEvent(true);
}

#pragma warning(pop)
