// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides data for the SelectionChanged event that occurs when a user
//      changes selected item(s) on a Selector based control

#pragma once

#include "SelectionChangedEventArgs.g.h"

namespace DirectUI
{
    // Provides /data for the SelectionChanged event that occurs when a user
    // changes selected item(s) on a Selector based control
    PARTIAL_CLASS(SelectionChangedEventArgs)
    {
    public:
        ~SelectionChangedEventArgs() override
        {
        }
        // should implement IList for AddedItems and RemovedItems properties.
        _Check_return_ HRESULT get_AddedItemsImpl(_Outptr_ wfc::IVector<IInspectable*>** pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_tpAddedItems.Get();
            AddRefInterface(*pValue);
        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT get_RemovedItemsImpl(_Outptr_ wfc::IVector<IInspectable*>** pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_tpRemovedItems.Get();
            AddRefInterface(*pValue);
        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT put_AddedItemsImpl(_In_ wfc::IVector<IInspectable*>* pValue)
        {
            SetPtrValue(m_tpAddedItems, pValue);
            return S_OK;
        }

        _Check_return_ HRESULT put_RemovedItemsImpl(_In_ wfc::IVector<IInspectable*>* pValue)
        {
            SetPtrValue(m_tpRemovedItems, pValue);
            return S_OK;
        }

    private:
        TrackerPtr<wfc::IVector<IInspectable*>> m_tpRemovedItems;
        TrackerPtr<wfc::IVector<IInspectable*>> m_tpAddedItems;
    };
}
