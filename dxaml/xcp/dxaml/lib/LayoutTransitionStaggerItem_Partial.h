// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutTransitionStaggerItem.g.h"

namespace DirectUI
{
    // note: is here because dxaml currently does not marshall these values correctly.
    PARTIAL_CLASS(LayoutTransitionStaggerItem)
    {
    private:
        wf::Rect m_bounds;
        wf::TimeSpan m_timespan;

    public:
        LayoutTransitionStaggerItem()
        {
            m_bounds = wf::Rect();
            m_timespan = wf::TimeSpan();
        }
        _Check_return_ HRESULT get_Bounds(_Out_ wf::Rect* pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_bounds;
        Cleanup: 
            RRETURN(hr);
        }

        _Check_return_ HRESULT put_Bounds(_In_ wf::Rect value)
        {
            m_bounds = value;
            RRETURN(S_OK);
        }

        _Check_return_ HRESULT get_StaggerTime(_Out_ wf::TimeSpan* pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_timespan;
        Cleanup: 
            RRETURN(hr);
        }

        _Check_return_ HRESULT put_StaggerTime(_In_ wf::TimeSpan value)
        {
            m_timespan = value;
            RRETURN(S_OK);
        }
    };
}


