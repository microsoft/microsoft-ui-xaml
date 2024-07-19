// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewGeneratorHost.h"
#include "ITreeBuilder.g.h"

namespace DirectUI
{

    class CalendarView;
    class CalendarPanel;
    class CalendarViewBaseItem;
    class CalendarViewDayItem;

    class CalendarViewGeneratorMonthViewHost
        : public CalendarViewGeneratorHost
        , public DirectUI::ITreeBuilder
    {
    protected:
        BEGIN_INTERFACE_MAP(CalendarViewGeneratorMonthViewHost, CalendarViewGeneratorHost)
            INTERFACE_ENTRY(CalendarViewGeneratorMonthViewHost, DirectUI::ITreeBuilder)
        END_INTERFACE_MAP(CalendarViewGeneratorMonthViewHost, CalendarViewGeneratorHost)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(DirectUI::ITreeBuilder)))
            {
                *ppObject = static_cast<DirectUI::ITreeBuilder*>(this);
            }
            else
            {
                return CalendarViewGeneratorHost::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
        CalendarViewGeneratorMonthViewHost();

        // ITreeBuuilder interface
        IFACEMETHOD(get_IsRegisteredForCallbacks)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsRegisteredForCallbacks)(_In_ BOOLEAN value) override;
        IFACEMETHOD(IsBuildTreeSuspended)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(BuildTree)(_Out_ BOOLEAN* pWorkLeft) override;
        IFACEMETHOD(ShutDownDeferredWork)() override;
        // End ITreeBuuilder interface


        _Check_return_ IFACEMETHOD(SetupContainerContentChangingAfterPrepare)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem,
            _In_ INT itemIndex,
            _In_ wf::Size measureSize) override;

        // the CCC version, for ListViewBase only
        _Check_return_ IFACEMETHOD(RegisterWorkFromArgs)(
            _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs) override { return S_OK; }

        _Check_return_ IFACEMETHOD(RegisterWorkForContainer)(
            _In_ xaml::IUIElement* pContainer) override;

        _Check_return_ IFACEMETHOD(PrepareItemContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem) override;

        _Check_return_ IFACEMETHOD(ClearContainerForItem)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem) override;

        _Check_return_ IFACEMETHOD(RaiseContainerContentChangingOnRecycle)(
            _In_ xaml::IUIElement* pContainer,
            _In_ IInspectable* pItem) override;

        _Check_return_ HRESULT GetContainer(
            _In_ IInspectable* pItem,
            _In_opt_ xaml::IDependencyObject* pRecycledContainer,
            _Outptr_ CalendarViewBaseItem** ppContainer) override;


        _Check_return_ HRESULT GetPossibleItemStrings(_Outptr_ const std::vector<wrl_wrappers::HString>** ppStrings) override;

        _Check_return_ HRESULT GetIsFirstItemInScope(_In_ int index, _Out_ bool* pIsFirstItemInScope) override;

        int GetMaximumScopeSize() override
        {
            return 31; // a month has 31 days in maximum.
        }

        INT64 GetAverageTicksPerUnit() override;

        _Check_return_ HRESULT GetUnit(_Out_ int* pValue) override;
        _Check_return_ HRESULT SetUnit(_In_ int value) override;
        _Check_return_ HRESULT AddUnits(_In_ int value) override;
        _Check_return_ HRESULT AddScopes(_In_ int value) override;
        _Check_return_ HRESULT GetFirstUnitInThisScope(_Out_ int* pValue) override;
        _Check_return_ HRESULT GetLastUnitInThisScope(_Out_ int* pValue) override;
        _Check_return_ HRESULT OnScopeChanged() override;

        _Check_return_ HRESULT UpdateLabel(_In_ CalendarViewBaseItem* pItem, _In_ bool isLabelVisible) override;

        _Check_return_ HRESULT CompareDate(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs, _Out_ int* pResult) override;

        // Returns true when the host uses the "Samoa Standard Time" time zone.
        bool IsUsingSamoaTimeZone(bool forceUpdate);

    private:
        _Check_return_ IFACEMETHOD(RegisterWorkFromCICArgs)(
            _In_ xaml_controls::ICalendarViewDayItemChangingEventArgs* pArgs);

        _Check_return_ HRESULT EnsureToBeClearedContainers();

        _Check_return_ HRESULT ProcessIncrementalVisualization(
            _In_ const ctl::ComPtr<BudgetManager>& spBudget,
            _In_ CalendarPanel* pCalendarPanel);

        _Check_return_ HRESULT ClearContainers(
            _In_ const ctl::ComPtr<BudgetManager>& spBudget);

        _Check_return_ HRESULT RemoveToBeClearedContainer(_In_ CalendarViewDayItem* pContainer);

    private:
        bool m_isUsingSamoaTimeZone{ false };            // True when the host uses the "Samoa Standard Time" time zone.
        bool m_isUsingSamoaTimeZoneInitialized{ false }; // True when m_isUsingSamoaTimeZone was evaluated.

        // the lowest phase container we have in the queue. -1 means nothing is in the queue
        INT64 m_lowestPhaseInQueue;

        // ITreeBuilder member
        // Fields.
        BOOLEAN m_isRegisteredForCallbacks;
        // End ITreeBuilder member

        // budget that we have to do other work, measured since the last tick
        UINT m_budget;

        // we keep a list of containers we have called clear on. Each time we call prepare, we remove it from this list
        // at the end of measure, we know to only execute code on containers that are left in this list.
        TrackerPtr<TrackerCollection<xaml_controls::CalendarViewDayItem*>> m_toBeClearedContainers;
    };

}
