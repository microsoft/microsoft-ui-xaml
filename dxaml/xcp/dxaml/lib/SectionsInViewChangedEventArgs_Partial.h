// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SectionsInViewChangedEventArgs.g.h"

namespace DirectUI
{
    // Provides /data for the SectionsInViewChanged event that occurs when a user
    // changes sections in view on a Hub control
    PARTIAL_CLASS(SectionsInViewChangedEventArgs)
    {
    public:
        ~SectionsInViewChangedEventArgs() override
        {
        }
        // should implement IList for AddedSections and RemovedSections properties.
        _Check_return_ HRESULT get_AddedSectionsImpl(_Outptr_ wfc::IVector<xaml_controls::HubSection*>** pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_tpAddedSections.Get();
            AddRefInterface(*pValue);
        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT get_RemovedSectionsImpl(_Outptr_ wfc::IVector<xaml_controls::HubSection*>** pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_tpRemovedSections.Get();
            AddRefInterface(*pValue);
        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT put_AddedSectionsImpl(_In_ wfc::IVector<xaml_controls::HubSection*>* pValue)
        {
            SetPtrValue(m_tpAddedSections, pValue);
            return S_OK;
        }

        _Check_return_ HRESULT put_RemovedSectionsImpl(_In_ wfc::IVector<xaml_controls::HubSection*>* pValue)
        {
            SetPtrValue(m_tpRemovedSections, pValue);
            return S_OK;
        }

    private:
        TrackerPtr<wfc::IVector<xaml_controls::HubSection*>> m_tpRemovedSections;
        TrackerPtr<wfc::IVector<xaml_controls::HubSection*>> m_tpAddedSections;
    };
}
