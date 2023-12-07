// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents budgetmanager to keep the uitick times

#pragma once

#include "BudgetManager.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(BudgetManager)
    {

    protected:
        BudgetManager();
        _Check_return_ HRESULT Initialize() override;

    public:
        _Check_return_ HRESULT StoreFrameTime(_In_ BOOL isBeginOfTick);

        _Check_return_ HRESULT GetElapsedMilliSecondsSinceLastUITickImpl(_Out_ INT* returnValue);

    private:
        LARGE_INTEGER m_freq;
        LARGE_INTEGER m_startFrameTime;
        LARGE_INTEGER m_endFrameTime;
        
    };
}

// The default elapsed time before deferring
#define BUDGET_MANAGER_DEFAULT_LIMIT 40
