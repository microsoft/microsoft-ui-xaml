// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BudgetManager.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

BudgetManager::BudgetManager()
    : m_freq()
{

}

_Check_return_ HRESULT BudgetManager::Initialize()
{
    BOOL success = FALSE;

    success = QueryPerformanceFrequency( &m_freq );

    return S_OK;
}

_Check_return_ HRESULT BudgetManager::StoreFrameTime(_In_ BOOL isBeginOfTick)
{
    HRESULT hr = S_OK;

    BOOL success = FALSE;
    
    if (isBeginOfTick)
    {
        success = QueryPerformanceCounter(&m_startFrameTime);
    }
    else
    {
        success = QueryPerformanceCounter(&m_endFrameTime);
    }
    // does all ARM hardware support this
    ASSERT(success);

    RRETURN(hr);
}


_Check_return_ HRESULT BudgetManager::GetElapsedMilliSecondsSinceLastUITickImpl(_Out_ INT* returnValue)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER lTimeCurrent = {};
    BOOL success = FALSE;

    // todo: switch between end and start time based on whether we trust the tick to be accurate

    success = QueryPerformanceCounter( &lTimeCurrent );

    if (success)
    {
        DOUBLE elapsedSeconds = static_cast<DOUBLE>(lTimeCurrent.QuadPart-m_endFrameTime.QuadPart)/static_cast<DOUBLE>(m_freq.QuadPart);

        *returnValue = static_cast<INT>(elapsedSeconds*1000);
    }
    else
    {
        // hardware that doesn't understand QPC?? Very unlikely, but in this case, let's not crash
        // and just return that no time has elapsed. This will trigger people relying on budget to see that they
        // have a lot of budget and atleast not have them encounter a situation where they do not get to perform
        // certain work.
        *returnValue = 0;
    }

    RRETURN(hr);
}
