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
        bool UseUnbudgetedContainerBuild() const
        {
            return m_useUnbudgetedContainerBuild;
        }

        void SetUseUnbudgetedContainerBuild(bool value)
        {
            m_useUnbudgetedContainerBuild = value;
        }
        
        _Check_return_ HRESULT StoreFrameTime(_In_ BOOL isBeginOfTick);

        _Check_return_ HRESULT GetElapsedMilliSecondsSinceLastUITickImpl(_Out_ INT* returnValue);

    private:
        LARGE_INTEGER m_freq{};
        LARGE_INTEGER m_startFrameTime{};
        LARGE_INTEGER m_endFrameTime{};
        
        // exceptionally set to True when (shift) tabbing into an unrealized item to give the BuildTreeService
        // enough time to prepare it to receive focus. This is specific to the Local/Cycle ListviewBase.TabNavigation.
        bool m_useUnbudgetedContainerBuild{ false };
    };
}

// The default elapsed time before deferring
#define BUDGET_MANAGER_DEFAULT_LIMIT 40
