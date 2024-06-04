// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlTraceLogging.h"
#include "ContentRoot.h"
#include "StableXbfIndexes.g.h"
#include "RuntimeProfiler.h"
#include "RuntimeProfiler_DynamicProfiler.h"
#include <new>
#include <algorithm>

#define DEFINE_PROFILEGROUP(name, group, size) \
    __pragma (message ("RuntimeProfiler Group:" #group "(" #size ")")) \
    CMethodProfileGroup<size>        name(group)

//  This library is used in different binaries, so this is always an external defined where the .rc file resides for that binary
extern const char *gFileVersion;

namespace RuntimeProfiler {

    void UninitializeRuntimeProfiler();

    struct FunctionTelemetryCount
    {
        volatile PCSTR      pszIdentifier;
        volatile LONG      *pInstanceCount;
        volatile UINT16     uStableXbfTypeIndex;
        volatile UINT16     uMethodIndex;
    };

    class CMethodProfileGroupBase
    {
    public:
        virtual void RegisterMethod(_In_ PCSTR pszIdentifier, _In_ UINT16 uTypeIndex, _In_ UINT16 uMethodIndex, _Inout_ volatile LONG *pCount) = 0;
        virtual void FireEvent(_In_ bool bSuspend) = 0;
    };

    template <size_t size>
    class CMethodProfileGroup: public CMethodProfileGroupBase
    {
    public:
        static const int TableSize = size;

        CMethodProfileGroup()
        :   m_cMethods(0)
        ,   m_group(PG_Class)
        {
            //  Note: We are declaring these objects in a global scope which
            //      basically means that anything we call here in the
            //      constructor needs to be safe to call during DllMain.
            ZeroMemory(&m_Counts, sizeof(m_Counts));
        }

        CMethodProfileGroup(_In_ ProfileGroup group)
        :   m_cMethods(0)
        ,   m_group(group)
        {
            //  Note: We are declaring these objects in a global scope which
            //      basically means that anything we call here in the
            //      constructor needs to be safe to call during DllMain.
            ZeroMemory(&m_Counts, sizeof(m_Counts));
        }

        ~CMethodProfileGroup()
        {
            //  Ditto from above.
            UninitializeRuntimeProfiler();
        }

        void InitializeGroup(_In_ ProfileGroup group)
        {
            //  Used when we use the default constructor...
            m_group = group;
        }

        void RegisterMethod(_In_ PCSTR pszIdentifier, _In_ UINT16 uTypeIndex, _In_ UINT16 uMethodIndex, _Inout_ volatile LONG *pCount) override
        {
            static_assert(sizeof(LONG) == sizeof(UINT32), "Since we're using InterlockedIncrement, make sure that this is the same size independent of build flavors.");

            //  Zero-based index
            LONG WriteIndex = ::InterlockedIncrement(&m_cMethods) - 1;

            if (WriteIndex < _countof(m_Counts))
            {
                m_Counts[WriteIndex].pszIdentifier       = pszIdentifier;
                m_Counts[WriteIndex].uStableXbfTypeIndex = uTypeIndex;
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

        //  This function returns the start of the string presentation of
        //  given number, it assumes the buffer passed has a sufficiently
        //  large buffer to accommodate the number.
        static PSTR ConstructNumber(_In_ UINT uNumber, _Out_ PSTR pszEnd)
        {
            *pszEnd = 0;  // null terminate!

            //  Special case 0
            if (0 == uNumber)
            {
                pszEnd--;
                *pszEnd = '0';
            }
            else
            {
                for (; uNumber; uNumber /= 10)
                {
                    pszEnd--;
                    *pszEnd = '0' + (uNumber % 10);
                }
            }

            return (pszEnd);
        }

        //  This function copies the source string to the destination string
        //  and return the pointer to the null terminator, it never writes past
        //  the given last character.
        static PSTR WriteString(_Out_ PSTR pszDst, _In_ PCSTR pszSrc, _Out_ PSTR pszLastChar)
        {
            for (; *pszSrc; pszSrc++, pszDst++)
            {
                *pszDst = *pszSrc;
                if (pszDst == pszLastChar)
                {
                    break;
                }
            }
            *pszDst = 0;

            return (pszDst);
        }

        //  Thin wrapper around the firing of the event, since we may now fire
        //  multiple events if our output buffer fills.
        static void FireEventRaw(PCSTR pszAPICounts, UINT32 uGroupId, UINT32 cMethods, bool bSuspend, bool bOverflow, bool bStringOverflow)
        {
            TraceLoggingWrite(
                g_hTraceProvider,
                "RuntimeProfiler",
                TraceLoggingDescription("XAML methods that have been called."),
                TraceLoggingString(gFileVersion, "BinaryVersion"),
                TraceLoggingString(pszAPICounts, "ApiCounts"),
                TraceLoggingUInt32(uGroupId, "ProfileGroupId"),
                TraceLoggingUInt32(cMethods,"TotalCount"),
                TraceLoggingBoolean(bSuspend, "OnSuspend"),
                TraceLoggingBoolean(bOverflow, "Overflow"),
                TraceLoggingBoolean(bStringOverflow, "StringOverflow"),
                TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        //  Note:  This method is now called during process detach, thus we
        //         may not call any API outside of kernel or risk acquiring the
        //         loader lock, so all string formatting is done manually.
        //         The only API's that we call are:
        //            InterlockedExchange()
        //            TraceLoggingWrite()
        void FireEvent(_In_ bool bSuspend) override
        {
            //  Note: We have a script that monitors the size of the string in
            //        the event and currently (01/2019) this tops out at < 450
            //        800 should be sufficient.  Moreover, if we overflow this
            //        string we simply fire multiple events.
            char        szOutput[800];

            char        szNumber[15];  //  MAX_INT is 4294967295
            bool        bOverflow = (m_cMethods >= _countof(m_Counts));
            UINT32      cMethods = (UINT32)m_cMethods;

            //  Used to construct output string.
            PSTR        pszOutput = &(szOutput[0]);
            PSTR        pszLastEntry = pszOutput;
            PSTR        pszLastChar = &(szOutput[_countof(szOutput) - 1]);
            PSTR        pszNumEnd = &(szNumber[_countof(szNumber) - 1]);

            for (UINT ii = 0; ii < cMethods; ii++)
            {
                UINT    cHits;

                if ((nullptr == m_Counts[ii].pInstanceCount) ||
                    ((nullptr == m_Counts[ii].pszIdentifier) && (0 == m_Counts[ii].uStableXbfTypeIndex)))
                {
                    //  In the middle of RegisterMethod on another thread,
                    //  we'll forgo logging this method for now and pick it
                    //  up on the next FireEvent().
                    continue;
                }

                //  Zeroing out AND getting current value.
                cHits = (UINT)(::InterlockedExchange(m_Counts[ii].pInstanceCount, 0));

                if (0 == cHits)
                {
                    //  Next!
                    continue;
                }

                for (;;)
                {
                    PSTR    pszWrite = pszLastEntry;

                    //  If not first entry, enter a separator...
                    if (pszWrite != pszOutput)
                    {
                        pszWrite = WriteString(pszWrite, ",", pszLastChar);
                    }

                    //  Are we using the strings or id's?
                    if (nullptr != m_Counts[ii].pszIdentifier)
                    {
                        //  This loop will create an entry that will look like this:
                        //  'TextBlock:15,Button:7'
                        pszWrite = WriteString(pszWrite, m_Counts[ii].pszIdentifier, pszLastChar);
                        pszWrite = WriteString(pszWrite, ":", pszLastChar);
                        pszWrite = WriteString(pszWrite, ConstructNumber(cHits, pszNumEnd), pszLastChar);
                    }
                    else
                    {
                        //  We're using id's instead.  The entry in the list will
                        //  look like '[type index|method index]:count'
                        pszWrite = WriteString(pszWrite, "[", pszLastChar);
                        pszWrite = WriteString(pszWrite, ConstructNumber(m_Counts[ii].uStableXbfTypeIndex, pszNumEnd), pszLastChar);
                        pszWrite = WriteString(pszWrite, "|", pszLastChar);
                        pszWrite = WriteString(pszWrite, ConstructNumber((m_Counts[ii].uMethodIndex), pszNumEnd), pszLastChar);
                        pszWrite = WriteString(pszWrite, "]:", pszLastChar);
                        pszWrite = WriteString(pszWrite, ConstructNumber(cHits, pszNumEnd), pszLastChar);
                    }

                    if (pszWrite == pszLastChar)
                    {
                        //  Our buffer is full...
                        *pszLastEntry = 0;  // truncating to last full entry we could accommodate
                        pszLastEntry = pszOutput;  // resetting the write buffer to process additional events
                        FireEventRaw(pszOutput, m_group, cMethods, bSuspend, bOverflow, true);  //  Firing the event

                        //  And loop through to process this entry again.
                    }
                    else
                    {
                        //  Wrote string.  Next!
                        pszLastEntry = pszWrite;
                        break;
                    }
                }
            }

            if (pszLastEntry != pszOutput)
            {
                //  We have something to log in the event.
                FireEventRaw(pszOutput, m_group, cMethods, bSuspend, bOverflow, false);
            }
        }

    private:
        FunctionTelemetryCount      m_Counts[TableSize];
        LONG                        m_cMethods;
        ProfileGroup                m_group;
    };  //  class CMethodProfileGroup

    //  Yes, we're declaring this as a global, the ctor/dtor are implemented
    //  very carefully and this will not create issues with DllMain().
    DEFINE_PROFILEGROUP(gGroupClasses, PG_Class, 50);

    struct
    {
        CMethodProfileGroupBase    *pGroup;
        const char                 *pszGroupName;
    } gProfileGroups[] =
    {
        { static_cast<CMethodProfileGroupBase*>(&gGroupClasses), "Classes" },
    };

    class CDynamicProfiler: public CDynamicProfilerBase
    {
    public:
        static CDynamicProfiler *CreateDynamicProfiler(_In_ ProfileGroup pg);

        void RegisterType(_In_ Parser::StableXbfTypeIndex uTypeIndex, _Inout_ volatile LONG *pCount) override
        {
            m_Profiler.RegisterMethod(nullptr, (UINT16)(uTypeIndex), RuntimeProfiler_Constructor, pCount);
        }

        void FireEvent(_In_ bool bSuspend)
        {
            m_Profiler.FireEvent(bSuspend);
        }

        LONG *GetCounterBuffer() override
        {
            return (m_Counters);
        }

        CDynamicProfiler           *m_pNext;

    private:
        CDynamicProfiler(_In_ ProfileGroup pg)
        :   m_pNext(nullptr)
        {
            //  Setting all the counters to -1, while this does it byte-wise
            //  it still works.
            memset(&(m_Counters[0]), -1, sizeof(m_Counters));

            m_Profiler.InitializeGroup(pg);
        }

        ~CDynamicProfiler();

        CMethodProfileGroup<500>                        m_Profiler;
        LONG                        m_Counters[Parser::StableXbfTypeCount];
    };

    //  Note:  An instance of this class is instantiated as a global, it may be
    //         accessed by multiple threads, and will be accessed during
    //         process detach, so we are avoiding constructs unless we know
    //         they are safe to call in these conditions.
    class CDynamicLists
    {
    public:
        CDynamicLists()
        :   m_pList(nullptr)
        {
        }

        void Add(_In_ CDynamicProfiler *pAdd)
        {
            pAdd->m_pNext = m_pList;
            m_pList = pAdd;
        }

        void Remove(_Inout_ CDynamicProfiler *pRemove)
        {
            CDynamicProfiler  **ppList = &m_pList;

            for (;*ppList != nullptr;)
            {
                if (pRemove == ::InterlockedCompareExchangePointer((PVOID*)(ppList), (PVOID)((*ppList)->m_pNext), (PVOID)(pRemove)))
                {
                    break;
                }

                ppList = &((*ppList)->m_pNext);
            }

            pRemove->FireEvent(true);
        }

        void FireEvent(_In_ bool bSuspend)
        {
            CDynamicProfiler    *pList = m_pList;

            for (;pList;pList = pList->m_pNext)
            {
                pList->FireEvent(bSuspend);
            }
        }

    private:
        CDynamicProfiler   *m_pList;
    };

    CDynamicLists  g_DynamicList;

    CDynamicProfiler *CDynamicProfiler::CreateDynamicProfiler(_In_ ProfileGroup pg)
    {
        CDynamicProfiler   *pNew = new CDynamicProfiler(pg);

        g_DynamicList.Add(pNew);

        return (pNew);
    }

    CDynamicProfiler::~CDynamicProfiler()
    {
        //  Remove ourselves from list...
        g_DynamicList.Remove(this);
    }

    DWORD const     EventFrequencyInMS = 20*60*1000;    //  20 minutes

    void FireEvent(_In_ bool bSuspend)
    {
        for (int ii = 0; ii < ARRAYSIZE(gProfileGroups); ii++)
        {
            CMethodProfileGroupBase    *pGroup = gProfileGroups[ii].pGroup;

            pGroup->FireEvent(bSuspend);
        }

        g_DynamicList.FireEvent(bSuspend);
    }

    VOID CALLBACK TPTimerCallback(PTP_CALLBACK_INSTANCE, PVOID, PTP_TIMER)
    {
        FireEvent(false);
    }

    PTP_TIMER   g_pTimer = NULL;

    BOOL CALLBACK CancelTimer(PINIT_ONCE /* InitOnce */, PVOID /* Parameter */, PVOID* /* context */)
    {
        if (NULL != g_pTimer)
        {
            //  Canceling timer.

            //  Note:  We're called on a global destructor, so we are not
            //    calling WaitForThreadpoolTimerCallbacks() to prevent
            //    deadlocks.
            SetThreadpoolTimer(g_pTimer, NULL, 0, 0);
            CloseThreadpoolTimer(g_pTimer);


            g_pTimer = NULL;
        }

        //  Either way, no active timer.
        return (TRUE);
    }

    void UninitializeRuntimeProfiler()
    {
        static INIT_ONCE    UninitProfiler = INIT_ONCE_STATIC_INIT;

        InitOnceExecuteOnce(&UninitProfiler, CancelTimer, NULL, NULL);
    }

    BOOL CALLBACK InitializeRuntimeProfiler(PINIT_ONCE /* InitOnce */, PVOID /* Parameter */, PVOID* /* context */)
    {
        g_pTimer = ::CreateThreadpoolTimer(TPTimerCallback, nullptr, nullptr);

        if (NULL != g_pTimer)
        {
            LARGE_INTEGER   lidueTime;
            FILETIME        ftdueTime;

            //  Setting periodic timer.  Using negative time in 100 nanosecond
            //  intervals to indicate relative time.
            lidueTime.QuadPart = -10000 * (LONGLONG)EventFrequencyInMS;

            ftdueTime.dwHighDateTime = (DWORD)(lidueTime.HighPart);
            ftdueTime.dwLowDateTime  = lidueTime.LowPart;

            //  Setting the callback window length to 60 seconds since the
            //  timing of the event is not critical
            SetThreadpoolTimer(g_pTimer, &ftdueTime, EventFrequencyInMS, 60 * 1000);
        }

        return ((NULL != g_pTimer)?TRUE:FALSE);
    }

    void RegisterMethod(_In_ ProfileGroup group, _In_ PCSTR pszIdentifier, _In_ UINT16 uTypeIndex, _In_ UINT16 uMethodIndex, _Inout_ volatile LONG *pCount)
    {
        static INIT_ONCE            InitProfiler = INIT_ONCE_STATIC_INIT;
        CMethodProfileGroupBase    *pGroup = gProfileGroups[(int)group].pGroup;

        InitOnceExecuteOnce(&InitProfiler, InitializeRuntimeProfiler, NULL, NULL);

        return (pGroup->RegisterMethod(pszIdentifier, uTypeIndex, uMethodIndex, pCount));
    }

    //  Not sure if IslandType can change, the group number in telemetry CANNOT
    //  change so we're mapping through the values.
    ProfileGroup MapToProfileGroup(CContentRoot::IslandType type)
    {
        switch (type)
        {
            //  Neither of these should be instrumented
            case CContentRoot::IslandType::Invalid:
            case CContentRoot::IslandType::Raw:
                return (PG_Class);

            case CContentRoot::IslandType::DesktopWindowContentBridge:
                return (PG_IslandDesktopWindowXamlSource);
        }

        IFCFAILFAST(E_NOTIMPL);

        //  To appease the compiler.
        return (PG_Class);
    }

    CDynamicProfilerBase *GetXamlIslandTypeProfiler(_In_ CContentRoot::IslandType IslandType)
    {
        switch (IslandType)
        {
            //  We don't care about these...
            case CContentRoot::IslandType::Invalid:
            case CContentRoot::IslandType::Raw:
                return (nullptr);

            case CContentRoot::IslandType::DesktopWindowContentBridge:
            {
                //  Do NOT combine these two cases!  The code looks the same
                //  but have static pointers that point to different instances
                //  of profilers
                static CDynamicProfiler *pProfiler = nullptr;

                if (nullptr == pProfiler)
                {
                    pProfiler = CDynamicProfiler::CreateDynamicProfiler(MapToProfileGroup(IslandType));
                }

                return (pProfiler);
            }
        }

        return (nullptr);
    }

} // namespace RuntimeProfiler

//  This will be exported by MUX Extension library
STDAPI_(void) SendTelemetryOnSuspend()
{
    RuntimeProfiler::FireEvent(true);
}
