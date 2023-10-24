// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IGeneratorHost.g.h"
#include "DirectManipulationStateChangeHandler.h"

namespace DirectUI
{
    enum DMManipulationState;
    class CalendarView;
    class CalendarViewBaseItem;
    class CalendarPanel;
    class ScrollViewer;

    class CalendarViewGeneratorHost
        : public IGeneratorHost
        , public wfc::IVector<IInspectable*>
        , public ctl::WeakReferenceSource
        , public DirectManipulationStateChangeHandler
    {

    protected:
        BEGIN_INTERFACE_MAP(CalendarViewGeneratorHost, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(CalendarViewGeneratorHost, IGeneratorHost)
            INTERFACE_ENTRY(CalendarViewGeneratorHost, wfc::IVector<IInspectable*>)
        END_INTERFACE_MAP(CalendarViewGeneratorHost, ctl::WeakReferenceSource)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(IGeneratorHost)))
            {
                *ppObject = static_cast<IGeneratorHost*>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IVector<IInspectable*>*>(this);
            }
            else
            {
                return ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
#pragma region IGeneratorHost interface

        _Check_return_ IFACEMETHOD(get_View)(
            _Outptr_ wfc::IVector<IInspectable*>** ppView) override;

        _Check_return_ IFACEMETHOD(get_CollectionView)(
            _Outptr_ xaml_data::ICollectionView** ppCollectionView) override;

        _Check_return_ IFACEMETHOD(IsItemItsOwnContainer)(
            _In_ IInspectable* pItem,
            _Out_ BOOLEAN* pIsOwnContainer) override;

        _Check_return_ IFACEMETHOD(GetContainerForItem)(
            _In_ IInspectable* pItem,
            _In_opt_ xaml::IDependencyObject* pRecycledContainer,
            _Outptr_ xaml::IDependencyObject** ppContainer) override;

        _Check_return_ IFACEMETHOD(PrepareItemContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem) override;

        _Check_return_ IFACEMETHOD(ClearContainerForItem)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem) override;

        _Check_return_ IFACEMETHOD(IsHostForItemContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _Out_ BOOLEAN* pIsHost) override;

        _Check_return_ IFACEMETHOD(GetGroupStyle)(
            _In_opt_ xaml_data::ICollectionViewGroup* pGroup,
            _In_ UINT level,
            _Out_ xaml_controls::IGroupStyle** ppGroupStyle) override;

        _Check_return_ IFACEMETHOD(SetIsGrouping)(_In_ BOOLEAN isGrouping) override;

        // we don't expose this publicly, there is an override for our own controls
        // to mirror the public api
        _Check_return_ IFACEMETHOD(GetHeaderForGroup)(
            _In_ IInspectable* pGroup,
            _Outptr_ xaml::IDependencyObject** ppContainer) override;

        _Check_return_ IFACEMETHOD(PrepareGroupContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ xaml_data::ICollectionViewGroup* pGroup) override;

        _Check_return_ IFACEMETHOD(ClearGroupContainerForGroup)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ xaml_data::ICollectionViewGroup* pItem) override;

        _Check_return_ IFACEMETHOD(SetupContainerContentChangingAfterPrepare)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem,
            _In_ INT itemIndex,
            _In_ wf::Size measureSize) override { return S_OK; }

         _Check_return_ IFACEMETHOD(RegisterWorkFromArgs)(
            _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs) override { return S_OK; }

        _Check_return_ IFACEMETHOD(RegisterWorkForContainer)(
            _In_ xaml::IUIElement* pContainer) override {  return S_OK; }

        _Check_return_ IFACEMETHOD(CanRecycleContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _Out_ BOOLEAN* pCanRecycleContainer) override;

        // During lookups of duplicate or null values, there might be a container that
        // the host can provide the ICG.
        _Check_return_ IFACEMETHOD(SuggestContainerForContainerFromItemLookup)(
            _Outptr_ xaml::IDependencyObject** ppContainer) override;

        _Check_return_ IFACEMETHOD(ShouldRaiseChoosingItemContainer)(
            _Out_ BOOLEAN* pShouldRaiseChoosingItemContainer) override { *pShouldRaiseChoosingItemContainer = false; return S_OK; }

        _Check_return_ IFACEMETHOD(RaiseChoosingItemContainer)(
            _In_ xaml_controls::IChoosingItemContainerEventArgs* pArgs) override { return S_OK; }

        _Check_return_ IFACEMETHOD(ShouldRaiseChoosingGroupHeaderContainer)(
            _Out_ BOOLEAN* pShouldRaiseChoosingGroupHeaderContainer) override { *pShouldRaiseChoosingGroupHeaderContainer = false; return S_OK; }

        _Check_return_ IFACEMETHOD(RaiseChoosingGroupHeaderContainer)(
            _In_ xaml_controls::IChoosingGroupHeaderContainerEventArgs* pArgs) override { return S_OK; }

        _Check_return_ IFACEMETHOD(RaiseContainerContentChangingOnRecycle)(
            _In_ xaml::IUIElement* pContainer,
            _In_ IInspectable* pItem) override { return S_OK; }

        _Check_return_ IFACEMETHOD(VirtualizationFinished)() override { return S_OK; }

        _Check_return_ IFACEMETHOD(OverrideContainerArrangeBounds)(
            _In_ INT index,
            _In_ wf::Rect suggestedBounds,
            _Out_ wf::Rect* newBounds) override
        {
            *newBounds = suggestedBounds;
            return S_OK;
        }

#pragma endregion

        // IVector<IInspectable*> interface
        IFACEMETHOD(GetAt)(_In_ UINT index, _Outptr_ IInspectable** item) override;
        IFACEMETHOD(get_Size)(_Out_ UINT* value) override;
        IFACEMETHOD(GetView)(_Outptr_opt_result_maybenull_ wfc::IVectorView<IInspectable*>** view) override;
        IFACEMETHOD(IndexOf)(_In_opt_ IInspectable* value, _Out_ UINT* index, _Out_ BOOLEAN* found) override;
        IFACEMETHOD(SetAt)(_In_ UINT index, _In_opt_ IInspectable* item) override;
        IFACEMETHOD(InsertAt)(_In_ UINT index, _In_ IInspectable* item) override;
        IFACEMETHOD(RemoveAt)(_In_ UINT index) override;
        IFACEMETHOD(Append)(_In_opt_ IInspectable* item) override;
        IFACEMETHOD(RemoveAtEnd)() override;
        IFACEMETHOD(Clear)() override;

        // End IVector<IInspectable*>

        // DirectManipulationStateChangeHandler implementation
        _Check_return_ HRESULT NotifyStateChange(
            _In_ DMManipulationState state,
            _In_ FLOAT xCumulativeTranslation,
            _In_ FLOAT yCumulativeTranslation,
            _In_ FLOAT zCumulativeFactor,
            _In_ FLOAT xCenter,
            _In_ FLOAT yCenter,
            _In_ BOOLEAN isInertial,
            _In_ BOOLEAN isTouchConfigurationActivated,
            _In_ BOOLEAN isBringIntoViewportConfigurationActivated) override;

    public:

        CalendarViewGeneratorHost();
        ~CalendarViewGeneratorHost() override;

        void SetOwner(_In_ CalendarView* pOwner) { m_pOwnerNoRef = pOwner; }

        CalendarPanel* GetPanel();
        ScrollViewer* GetScrollViewer();

        _Check_return_ HRESULT AttachVisibleIndicesUpdatedEvent();
        _Check_return_ HRESULT DetachVisibleIndicesUpdatedEvent();

        _Check_return_ HRESULT AttachScrollViewerFocusEngagedEvent();
        _Check_return_ HRESULT DetachScrollViewerFocusEngagedEvent();

        _Check_return_ HRESULT OnPrimaryPanelDesiredSizeChanged();

        // return a pointer to the vector of possible strings, however the vector will be destroyed once this host is destoryed
        // so caller should only keep the return value only when this host is alive.
        virtual _Check_return_ HRESULT GetPossibleItemStrings(_Outptr_ const std::vector<wrl_wrappers::HString>** ppStrings) = 0;

        void ResetPossibleItemStrings()
        {
            m_possibleItemStrings.clear();
        }

        _Check_return_ HRESULT SetPanel(_In_opt_ xaml_primitives::ICalendarPanel* pPanel);
        _Check_return_ HRESULT SetScrollViewer(_In_opt_ xaml_controls::IScrollViewer* pScrollViewer);

        std::array<int, 2>& GetLastVisibleIndicesPairRef() { return m_lastVisibleIndicesPair; }

        CalendarView* GetOwner() { ASSERT(m_pOwnerNoRef); return m_pOwnerNoRef; }

        wg::ICalendar* GetCalendar();

        _Check_return_ HRESULT GetFirstDateOfNextScope(
            _In_ wf::DateTime dateOfFirstVisibleItem,
            _In_ bool forward,
            _Out_ wf::DateTime* pFirstDateOfNextScope);

        _Check_return_ HRESULT UpdateScope(_In_ wf::DateTime firstDate, _In_ wf::DateTime lastDate, _Out_ bool* isScopeChanged);

        wf::DateTime GetMinDateOfCurrentScope() { return m_minDateOfCurrentScope; }
        wf::DateTime GetMaxDateOfCurrentScope() { return m_maxDateOfCurrentScope; }
        wrl_wrappers::HString* GetHeaderTextOfCurrentScope() { return &m_pHeaderText; }

        _Check_return_ HRESULT ComputeSize();

        _Check_return_ HRESULT CalculateOffsetFromMinDate(_In_ wf::DateTime date, _Out_ int* pIndex);

        virtual INT64 GetAverageTicksPerUnit() = 0;

        virtual _Check_return_ HRESULT GetIsFirstItemInScope(_In_ int index, _Out_ bool* pIsFirstItemInScope) = 0;

        virtual int GetMaximumScopeSize() = 0;

        // one scope is one Month for MonthView, one Year for YearView, one Decade for DecadeView.
        _Check_return_ HRESULT AddScopes(_Inout_ wf::DateTime& date, _In_ int scopes);

        // one unit is one Day for MonthView, one Month for YearView, one Year for DecadeView.
        _Check_return_ HRESULT AddUnits(_Inout_ wf::DateTime& date, _In_ int units);

        _Check_return_ HRESULT GetDateAt(_In_ UINT index, _Out_ wf::DateTime* pDate);

        virtual _Check_return_ HRESULT UpdateLabel(_In_ CalendarViewBaseItem* pItem, _In_ bool isLabelVisible) = 0;

        virtual _Check_return_ HRESULT CompareDate(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs, _Out_ int* pResult) = 0;

        void ResetScope();

    protected:

        xaml::Thickness GetItemMargin() const;
        xaml::Thickness GetItemFocusVisualMargin() const;

        virtual _Check_return_ HRESULT GetContainer(
            _In_ IInspectable* pItem,
            _In_opt_ xaml::IDependencyObject* pRecycledContainer,
            _Outptr_ CalendarViewBaseItem** ppContainer) = 0;

        virtual _Check_return_ HRESULT GetUnit(_Out_ int* pValue) = 0;
        virtual _Check_return_ HRESULT SetUnit(_In_ int value) = 0;
        virtual _Check_return_ HRESULT AddUnits(_In_ int value) = 0;
        virtual _Check_return_ HRESULT AddScopes(_In_ int value) = 0;
        virtual _Check_return_ HRESULT GetFirstUnitInThisScope(_Out_ int* pValue) = 0;
        virtual _Check_return_ HRESULT GetLastUnitInThisScope(_Out_ int* pValue) = 0;
        virtual _Check_return_ HRESULT OnScopeChanged() = 0;

    public:
        _Check_return_ HRESULT AdjustToFirstUnitInThisScope(_Out_ wf::DateTime* pDate, _Out_opt_ int* pUnit = nullptr);
        _Check_return_ HRESULT AdjustToLastUnitInThisScope(_Out_ wf::DateTime* pDate, _Out_opt_ int* pUnit = nullptr);

    protected:

        wf::DateTime m_minDateOfCurrentScope;
        wf::DateTime m_maxDateOfCurrentScope;
        std::pair<wf::DateTime, int> m_lastVisitedDateAndIndex;
        wrl_wrappers::HString m_pHeaderText;
        UINT m_size;

        CalendarView* m_pOwnerNoRef;

        TrackerPtr<xaml_primitives::ICalendarPanel> m_tpPanel;

        // stores all the possible strings in this view, we need them to determine the biggest item
        // for MonthView, it will contain 31 day strings
        // for YearView, it will contain up to 13 month strings
        // for DecadeView, we can't include all the possible string, here we'll use only one string.
        std::vector<wrl_wrappers::HString> m_possibleItemStrings;

    private:
        TrackerPtr<xaml_controls::IScrollViewer> m_tpScrollViewer;
        ctl::EventPtr<VisibleIndicesUpdatedEventCallback> m_epVisibleIndicesUpdatedHandler;
        ctl::EventPtr<ControlFocusEngagedEventCallback> m_epScrollViewerFocusEngagedEventHandler;

        // remember the last Visible indices, we need to update OutOfScope state from that range
        std::array<int, 2> m_lastVisibleIndicesPair;
    };
}
