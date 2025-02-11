// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "Panel.g.h"
#include "IChildTransitionContextProvider.g.h"
#include "IContainerContentChangingIterator.g.h"
#include "ICustomGeneratorItemsHost.g.h"
#include "IGeneratorHost.g.h"
#include "IGroupHeaderMapping.g.h"
#include "IItemContainerGenerator2.g.h"
#include "IItemLookupPanel.g.h"
#include "IKeyboardHeaderNavigationPanel.g.h"
#include "IKeyboardNavigationPanel.g.h"
#include "IPaginatedPanel.g.h"
#include "IScrollInfo.g.h"
#include "ITreeBuilder.g.h"

#define __ModernCollectionBasePanel_GUID "6d7d2d4d-4fb3-4f85-9aa1-a1c2950178fe"

namespace DirectUI
{
    class ModernCollectionBasePanel;
    class UIElement;

    class __declspec(novtable) ModernCollectionBasePanelGenerated:
        public DirectUI::Panel
        , public ABI::Microsoft::UI::Xaml::Controls::IInsertionPanel
        , public ABI::Microsoft::UI::Xaml::Controls::IItemContainerMapping
        , public ABI::Microsoft::UI::Xaml::Controls::Primitives::IScrollSnapPointsInfo
        , public DirectUI::IChildTransitionContextProvider
        , public DirectUI::ICustomGeneratorItemsHost
        , public DirectUI::IGroupHeaderMapping
        , public DirectUI::IItemContainerGenerator2
        , public DirectUI::IItemLookupPanel
        , public DirectUI::IKeyboardHeaderNavigationPanel
        , public DirectUI::IKeyboardNavigationPanel
        , public DirectUI::IPaginatedPanel
        , public DirectUI::ITreeBuilder
    {
        friend class DirectUI::ModernCollectionBasePanel;


        BEGIN_INTERFACE_MAP(ModernCollectionBasePanelGenerated, DirectUI::Panel)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, ABI::Microsoft::UI::Xaml::Controls::IInsertionPanel)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, ABI::Microsoft::UI::Xaml::Controls::IItemContainerMapping)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, ABI::Microsoft::UI::Xaml::Controls::Primitives::IScrollSnapPointsInfo)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::IChildTransitionContextProvider)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::ICustomGeneratorItemsHost)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::IGroupHeaderMapping)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::IItemContainerGenerator2)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::IItemLookupPanel)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::IKeyboardHeaderNavigationPanel)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::IKeyboardNavigationPanel)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::IPaginatedPanel)
            INTERFACE_ENTRY(ModernCollectionBasePanelGenerated, DirectUI::ITreeBuilder)
        END_INTERFACE_MAP(ModernCollectionBasePanelGenerated, DirectUI::Panel)

    public:
        ModernCollectionBasePanelGenerated();
        ~ModernCollectionBasePanelGenerated() override;

        // Event source typedefs.
        typedef CEventSource<ABI::Windows::Foundation::IEventHandler<IInspectable*>, IInspectable, IInspectable> VisibleIndicesUpdatedEventSourceType;
        typedef CEventSource<ABI::Windows::Foundation::IEventHandler<IInspectable*>, IInspectable, IInspectable> HorizontalSnapPointsChangedEventSourceType;
        typedef CEventSource<ABI::Windows::Foundation::IEventHandler<IInspectable*>, IInspectable, IInspectable> VerticalSnapPointsChangedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ModernCollectionBasePanel;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ModernCollectionBasePanel;
        }

        // Properties.
        IFACEMETHOD(get_AreHorizontalSnapPointsRegular)(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT get_AreStickyGroupHeadersEnabledBase(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_AreStickyGroupHeadersEnabledBase(BOOLEAN value);
        IFACEMETHOD(get_AreVerticalSnapPointsRegular)(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT get_FirstCacheGroupIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_FirstCacheGroupIndexBase(INT value);
        _Check_return_ HRESULT get_FirstCacheIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_FirstCacheIndexBase(INT value);
        _Check_return_ HRESULT get_FirstVisibleGroupIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_FirstVisibleGroupIndexBase(INT value);
        _Check_return_ HRESULT get_FirstVisibleIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_FirstVisibleIndexBase(INT value);
        IFACEMETHOD(get_IsRegisteredForCallbacks)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsRegisteredForCallbacks)(BOOLEAN value) override;
        _Check_return_ HRESULT get_LastCacheGroupIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_LastCacheGroupIndexBase(INT value);
        _Check_return_ HRESULT get_LastCacheIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_LastCacheIndexBase(INT value);
        _Check_return_ HRESULT get_LastVisibleGroupIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_LastVisibleGroupIndexBase(INT value);
        _Check_return_ HRESULT get_LastVisibleIndexBase(_Out_ INT* pValue);
        _Check_return_ HRESULT put_LastVisibleIndexBase(INT value);
        _Check_return_ HRESULT get_PanningDirectionBase(_Out_ ABI::Microsoft::UI::Xaml::Controls::PanelScrollingDirection* pValue);
        _Check_return_ HRESULT put_PanningDirectionBase(ABI::Microsoft::UI::Xaml::Controls::PanelScrollingDirection value);

        // Events.
        _Check_return_ HRESULT GetHorizontalSnapPointsChangedEventSourceNoRef(_Outptr_ HorizontalSnapPointsChangedEventSourceType** ppEventSource);
        IFACEMETHOD(add_HorizontalSnapPointsChanged)(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_HorizontalSnapPointsChanged)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetVerticalSnapPointsChangedEventSourceNoRef(_Outptr_ VerticalSnapPointsChangedEventSourceType** ppEventSource);
        IFACEMETHOD(add_VerticalSnapPointsChanged)(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_VerticalSnapPointsChanged)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetVisibleIndicesUpdatedEventSourceNoRef(_Outptr_ VisibleIndicesUpdatedEventSourceType** ppEventSource);
        _Check_return_ HRESULT add_VisibleIndicesUpdated(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT remove_VisibleIndicesUpdated(EventRegistrationToken token);

        // Methods.
        IFACEMETHOD(BuildTree)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(ContainerFromIndex)(INT index, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppReturnValue) override;
        IFACEMETHOD(ContainerFromItem)(_In_opt_ IInspectable* pItem, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppReturnValue) override;
        IFACEMETHOD(DisconnectItemsHost)() override;
        IFACEMETHOD(FindRecyclingCandidate)(INT index, _Out_ BOOLEAN* pHasMatchingCandidate) override;
        IFACEMETHOD(GenerateContainerAtIndex)(INT index, _Outptr_ ABI::Microsoft::UI::Xaml::IUIElement** ppReturnValue) override;
        IFACEMETHOD(GenerateHeaderAtGroupIndex)(INT index, _Outptr_ ABI::Microsoft::UI::Xaml::IUIElement** ppReturnValue) override;
        IFACEMETHOD(GetChildTransitionBounds)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ ABI::Windows::Foundation::Rect* pReturnValue) = 0;
        IFACEMETHOD(GetChildTransitionContext)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, INT LayoutTickId, _Out_ DirectUI::ThemeTransitionContext* pReturnValue) = 0;
        IFACEMETHOD(GetClosestElementInfo)(ABI::Windows::Foundation::Point position, _Out_ ABI::Microsoft::UI::Xaml::Controls::Primitives::ElementInfo* pReturnValue) = 0;
        IFACEMETHOD(GetContainerRecycleQueueEmpty)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(GetContainersForIncrementalVisualization)(_Outptr_ DirectUI::IContainerContentChangingIterator** ppReturnValue) override;
        IFACEMETHOD(GetGroupHeaderMapping)(_Outptr_ DirectUI::IGroupHeaderMapping** ppReturnValue) override;
        IFACEMETHOD(GetHeaderRecycleQueueEmpty)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(GetInsertionIndex)(ABI::Windows::Foundation::Point position, _Out_ INT* pReturnValue) = 0;
        IFACEMETHOD(GetInsertionIndexes)(ABI::Windows::Foundation::Point position, _Out_ INT* pFirst, _Out_ INT* pSecond) override;
        IFACEMETHOD(GetIrregularSnapPoints)(ABI::Microsoft::UI::Xaml::Controls::Orientation orientation, ABI::Microsoft::UI::Xaml::Controls::Primitives::SnapPointsAlignment alignment, _Outptr_ ABI::Windows::Foundation::Collections::IVectorView<FLOAT>** ppReturnValue) override;
        IFACEMETHOD(GetItemContainerMapping)(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IItemContainerMapping** ppReturnValue) override;
        IFACEMETHOD(GetItemsBounds)(_Out_ ABI::Windows::Foundation::Rect* pReturnValue) = 0;
        IFACEMETHOD(GetItemsPerPage)(_In_ DirectUI::IScrollInfo* pScrollInfo, _Out_ DOUBLE* pReturnValue) = 0;
        IFACEMETHOD(GetLastItemIndexInViewport)(_In_ DirectUI::IScrollInfo* pScrollInfo, _Out_ INT* pReturnValue) = 0;
        IFACEMETHOD(GetQueueLength)(_Out_ UINT* pReturnValue) override;
        IFACEMETHOD(GetRegularSnapPoints)(ABI::Microsoft::UI::Xaml::Controls::Orientation orientation, ABI::Microsoft::UI::Xaml::Controls::Primitives::SnapPointsAlignment alignment, _Out_ FLOAT* pOffset, _Out_ FLOAT* pReturnValue) override;
        IFACEMETHOD(GetTargetHeaderIndexFromNavigationAction)(UINT sourceIndex, ABI::Microsoft::UI::Xaml::Controls::KeyNavigationAction action, _Out_ UINT* pComputedTargetIndex, _Out_ BOOLEAN* pActionHandled) = 0;
        IFACEMETHOD(GetTargetIndexFromNavigationAction)(UINT sourceIndex, ABI::Microsoft::UI::Xaml::Controls::ElementType sourceType, ABI::Microsoft::UI::Xaml::Controls::KeyNavigationAction action, BOOLEAN allowWrap, UINT itemIndexHintForHeaderNavigation, _Out_ UINT* pComputedTargetIndex, _Out_ ABI::Microsoft::UI::Xaml::Controls::ElementType* pComputedTargetType, _Out_ BOOLEAN* pActionValidForSourceIndex) = 0;
        IFACEMETHOD(GroupFromHeader)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pHeader, _Outptr_ IInspectable** ppReturnValue) override;
        IFACEMETHOD(GroupHeaderContainerFromItemContainer)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pItemContainer, _Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppReturnValue) override;
        IFACEMETHOD(HeaderFromGroup)(_In_ IInspectable* pGroup, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppReturnValue) override;
        IFACEMETHOD(HeaderFromIndex)(INT index, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppReturnValue) override;
        IFACEMETHOD(IndexFromContainer)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pContainer, _Out_ INT* pReturnValue) override;
        IFACEMETHOD(IndexFromHeader)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pHeader, BOOLEAN excludeHiddenEmptyGroups, _Out_ INT* pReturnValue) override;
        IFACEMETHOD(IsBuildTreeSuspended)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(IsCollectionMutatingFast)(_Out_ BOOLEAN* pReturnValue) = 0;
        IFACEMETHOD(IsLayoutBoundary)(INT index, _Out_ BOOLEAN* pIsLeftBoundary, _Out_ BOOLEAN* pIsTopBoundary, _Out_ BOOLEAN* pIsRightBoundary, _Out_ BOOLEAN* pIsBottomBoundary) = 0;
        IFACEMETHOD(ItemFromContainer)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pContainer, _Outptr_ IInspectable** ppReturnValue) override;
        IFACEMETHOD(NotifyOfItemsChanged)(_In_ ABI::Windows::Foundation::Collections::IVectorChangedEventArgs* pE) override;
        IFACEMETHOD(NotifyOfItemsChanging)(_In_ ABI::Windows::Foundation::Collections::IVectorChangedEventArgs* pE) override;
        IFACEMETHOD(NotifyOfItemsReordered)(UINT count) override;
        IFACEMETHOD(RecycleAllContainers)() override;
        IFACEMETHOD(RecycleAllHeaders)() override;
        IFACEMETHOD(RecycleContainer)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pContainer) override;
        IFACEMETHOD(RecycleHeader)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pHeader) override;
        IFACEMETHOD(Refresh)() override;
        IFACEMETHOD(RegisterItemsHost)(_In_ DirectUI::IGeneratorHost* pHost) override;
        IFACEMETHOD(ShutDownDeferredWork)() override;
        IFACEMETHOD(SupportsKeyNavigationAction)(ABI::Microsoft::UI::Xaml::Controls::KeyNavigationAction action, _Out_ BOOLEAN* pReturnValue) = 0;
        IFACEMETHOD(TryRecycleContainer)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pContainer, _Out_ BOOLEAN* pReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        _Check_return_ HRESULT EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo) override;
        _Check_return_ HRESULT EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler) override;

    private:

        // Fields.
        INT m_firstCacheGroupIndexBase;
        INT m_firstCacheIndexBase;
        INT m_firstVisibleGroupIndexBase;
        INT m_firstVisibleIndexBase;
        BOOLEAN m_isRegisteredForCallbacks;
        INT m_lastCacheGroupIndexBase;
        INT m_lastCacheIndexBase;
        INT m_lastVisibleGroupIndexBase;
        INT m_lastVisibleIndexBase;
    };
}

#include "ModernCollectionBasePanel_Partial.h"

