// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Manages the sequence of navigated content and the navigations
//      between them. Serializes and deserializes the navigation entries
//      using Windows storage.

#pragma once

namespace DirectUI
{
    class PageStackEntry;
    class NavigationCache;
    class PageStackEntryTrackerCollection;

    class NavigationHistory : public ctl::WeakReferenceSource
    {
    private:
        BOOLEAN m_isNavigationPending;

        BOOLEAN m_isSetNavigationStatePending;

        // This can be NULL for cases where we skip navigating to current, when NULL it won't be added to the BackStack or ForwardStack.
        TrackerPtr<PageStackEntry> m_tpCurrentPageStackEntry;
        TrackerPtr<PageStackEntry> m_tpPendingPageStackEntry;

        TrackerPtr<PageStackEntryTrackerCollection> m_tpForwardStack;
        TrackerPtr<PageStackEntryTrackerCollection> m_tpBackStack;

        xaml_controls::IFrame *m_pIFrame;

        xaml::Navigation::NavigationMode m_navigationMode;

    protected:

        NavigationHistory();
        ~NavigationHistory() override;

    public:
        static _Check_return_ HRESULT
        Create(
            _In_ xaml_controls::IFrame *pIFrame,
            _Outptr_ NavigationHistory **ppNavigationHistory);

        _Check_return_ HRESULT
        NavigatePrevious();

        _Check_return_ HRESULT
        NavigateNext();

        _Check_return_ HRESULT
        NavigateNew(
            _In_ HSTRING descriptor,
            _In_opt_ IInspectable *pParameter,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo);

        _Check_return_ HRESULT
        GetBackStack(
            _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue);

        _Check_return_ HRESULT
        GetForwardStack(
            _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue);

        _Check_return_ HRESULT
        GetCurrentPageStackEntry(
            _Outptr_result_maybenull_ PageStackEntry **ppPageStackEntry);

        _Check_return_ HRESULT
        GetPendingPageStackEntry(
            _Outptr_ PageStackEntry **ppPageStackEntry);

        _Check_return_ HRESULT
        GetPendingNavigationMode(
            _Out_ xaml::Navigation::NavigationMode *pNavigationMode);

        _Check_return_ HRESULT
        GetCurrentNavigationMode(
            _Out_ xaml::Navigation::NavigationMode *pNavigationMode);

        _Check_return_ HRESULT
        CommitNavigation();

        _Check_return_ HRESULT
        CommitSetNavigationState(_In_ NavigationCache *pNavigationCache);

        _Check_return_ HRESULT
        ValidateCanChangePageStack();

        _Check_return_ HRESULT
        ValidateCanInsertEntry(_In_ PageStackEntry* pEntry);

        _Check_return_ HRESULT
        ValidateCanClearPageStack();

        _Check_return_ HRESULT
        ResetPageStackEntries(
            _In_ BOOLEAN isBackStack);

        _Check_return_ HRESULT
        OnPageStackChanging(
            _In_ BOOLEAN isBackStack,
            _In_ wfc::CollectionChange action,
            _In_ UINT index,
            _In_opt_ PageStackEntry* pEntry);

        _Check_return_ HRESULT
        OnPageStackChanged(
            _In_ BOOLEAN isBackStack,
            _In_ wfc::CollectionChange action,
            _In_ UINT index);

        _Check_return_ HRESULT
        GetNavigationState(_Out_ HSTRING* pNavigationState);

        _Check_return_ HRESULT
        SetNavigationState(_In_ HSTRING navigationState, _In_ BOOLEAN suppressNavigate);

    private:
        void DeInit();

        _Check_return_ HRESULT
        ClearNavigationHistory();

        _Check_return_ HRESULT
        WritePageStackEntryToString(
            _In_ PageStackEntry *pPageStackEntry,
            _Inout_ string &buffer);

        _Check_return_ HRESULT
        ReadPageStackEntryFromString(
            _In_ string &buffer,
            _In_ size_t currentPosition,
            _In_ PageStackEntry **ppPageStackEntry,
            _Out_ size_t *pNextPosition);
    };
}
