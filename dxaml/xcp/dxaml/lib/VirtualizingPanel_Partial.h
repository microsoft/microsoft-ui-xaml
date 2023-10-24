// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingPanel.g.h"

namespace DirectUI
{
    // Represents a VirtualizingPanel.
    // 
    PARTIAL_CLASS(VirtualizingPanel)
    {
    public:
        VirtualizingPanel();
        ~VirtualizingPanel() override;

        // Properties.
        _Check_return_ HRESULT get_ItemContainerGeneratorImpl(
            _Outptr_ xaml_controls::IItemContainerGenerator** pValue);

        // Virtual methods.
        _Check_return_ HRESULT OnItemsChangedImpl(
            _In_ IInspectable* sender, 
            _In_ xaml_primitives::IItemsChangedEventArgs* args);

        _Check_return_ HRESULT OnClearChildrenImpl();

        _Check_return_ HRESULT BringIndexIntoViewImpl(_In_ INT index);

        // Protected methods.
        _Check_return_ HRESULT AddInternalChildImpl(
            _In_ xaml::IUIElement* child);

        _Check_return_ HRESULT InsertInternalChildImpl(
            _In_ INT index, 
            _In_ xaml::IUIElement* child);

        _Check_return_ HRESULT RemoveInternalChildRangeImpl(_In_ INT index, _In_ INT range);

        virtual _Check_return_ HRESULT ScrollIntoView(
            _In_ UINT index, 
            _In_ BOOLEAN isGroupItemIndex,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment)        
        {
            RRETURN(S_OK);
        };

        // The method is being accessed from GetDesiredItemWidth\GetDesiredItemHeight of FlipView

        // During first measure of FlipView we have to know the Layout size which we are passing to
        // FlipView items. If we don't pass it then the Virtualizing panel might realize all items
        // which breaks virtualization.         
        wf::Size GetMeasureAvailableSize()
        {
            return m_LastSetAvailableSize;
        }

    protected:
        virtual _Check_return_ HRESULT OnItemsChangedInternal(_In_ IInspectable* sender, _In_ xaml_primitives::IItemsChangedEventArgs* args);

        _Check_return_ HRESULT GetIndexInGroupView(
            _In_ UINT index,
            _Out_ DOUBLE& groupIndex);

    public:
        // need access from ItemsControl
        virtual _Check_return_ HRESULT OnClearChildrenInternal();
        
        // Framework version of this method that allows for passing in an itemscontrol
        // if we already know about it (the look was a huge perf hit).
        _Check_return_ HRESULT GetItemContainerGenerator(
            _Outptr_ xaml_controls::IItemContainerGenerator** pValue,
            _In_ xaml_controls::IItemsControl* pItemsControlHint);

        INT32 GetItemsHostValidatedTick() { return m_itemsHostValidatedTick; }
        void SetItemsHostValidatedTick(_In_ INT32 tick) { m_itemsHostValidatedTick = tick; }

    private:
        _Check_return_ HRESULT get_Generator(
            _Outptr_ xaml_controls::IItemContainerGenerator** pValue,
            _In_opt_ xaml_controls::IItemsControl* pItemsControlHint = NULL);

        _Check_return_ HRESULT OnItemsChangedHandler(
            _In_ IInspectable* sender, 
            _In_ xaml_primitives::IItemsChangedEventArgs* args);

        _Check_return_ HRESULT FillBuffers(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);
        
        TrackerPtr<xaml::IDispatcherTimer> m_tpFillBuffersTimer;
        EventRegistrationToken m_ItemsChangedToken;
        BOOL m_bGeneratorHooked;
        
        INT32 m_itemsHostValidatedTick;
    protected:

        _Check_return_ HRESULT SetFillBuffersTimer();
        
        _Check_return_ HRESULT SetupItemBoundsClip();

        BOOL m_isGeneratingNewContainers;

        // Used to keep track of available size sent during last Measure pass.
        // This value is used to calculate how many items should be generated when new items get added.
        wf::Size m_LastSetAvailableSize;

        // This value is used to generate and measure containers when new items get added/replaced.
        wf::Size m_LastSetChildLayoutSlotSize;
                
        // Tracks if items should be measure in buffers. 
        // Used for perf optimization for startup.
        BOOLEAN m_bShouldMeasureBuffers;
        EventRegistrationToken m_fillBufferTimerTickToken;

        INT m_IndexToEnsureInView;                        // index of item which is below currently realized items and needs to be scrolled into view.
        static const XFLOAT ExtraContainerArrangeOffset;  // The offset at which off screen elements are arranged.
    };
}
