// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a PageStackEntryTrackerCollection.
//      Wraps TrackerCollection<PageStackEntry> and provides VectorChanging
//      and VectorChanged events.

#pragma once

namespace DirectUI
{

    class NavigationHistory;

    class PageStackEntryTrackerCollection:
        public TrackerCollection<xaml::Navigation::PageStackEntry*>
    {

    private:
        // Flag indicating whether this collection corresponds to the BackStack or the ForwardStack.
        BOOLEAN m_isBackStack{};

        // Weak reference to the NavigationHistory that owns this PageStack collection.
        ctl::WeakRefPtr m_wrNavigationHistory;

    public:

        IFACEMETHOD (SetAt)(_In_ UINT index, _In_opt_ T_abi item) override;
        IFACEMETHOD (InsertAt)(_In_ UINT index, _In_ T_abi item) override;
        IFACEMETHOD (RemoveAt)(_In_ UINT index) override;
        IFACEMETHOD (Append)(_In_opt_ T_abi item) override;
        IFACEMETHOD (RemoveAtEnd)() override;
        IFACEMETHOD (Clear)() override;


    private:
        _Check_return_ HRESULT
        OnVectorChanging(
            _In_ wfc::CollectionChange action,
            _In_ UINT index,
            _In_opt_ T_abi item);

        _Check_return_ HRESULT
        OnVectorChanged(
            _In_ wfc::CollectionChange action,
            _In_ UINT index);

        _Check_return_ HRESULT
        GetNavigationHistory(
            _Outptr_ NavigationHistory** ppNavigationHistory);

        _Check_return_ HRESULT
        SetNavigationHistory(
            _In_ NavigationHistory* pNavigationHistory);

    public:
        _Check_return_ HRESULT
        Init(
            _In_ NavigationHistory* pNavigationHistory,
            _In_ BOOLEAN isBackStack);

        _Check_return_ HRESULT
        AppendInternal(
            _In_opt_ T_abi item);

        _Check_return_ HRESULT
        RemoveAtEndInternal();

        _Check_return_ HRESULT
        ClearInternal();

        _Check_return_ HRESULT
        GetAtEnd(_Outptr_ T_abi *item);
    };
}
