// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


namespace DirectUI
{
    interface IScrollOwner;

    // Simple struct to track scroll offsets in the ScrollData.
    struct ScrollVector
    {
    public:
        DOUBLE X;
        DOUBLE Y;
    };
    
    // Representation of a ScrollContentPresenter's state.
    class ScrollData
    {
    public:
        // Whether the view can be scrolled horizontally.
        BOOLEAN m_canHorizontallyScroll{ FALSE };

        // Whether the view can be scrolled vertically.
        BOOLEAN m_canVerticallyScroll{ FALSE };

        // Minimal value of scroll offset of content.  
        ScrollVector m_MinOffset;
        
        // Computed offset based on _offset set by the IScrollInfo methods.  Set
        // at the end of a successful Measure pass.  This is the offset used by
        // Arrange and exposed externally.  Thus an offset set by PageDown via
        // IScrollInfo isn't reflected publicly (e.g. via the VerticalOffset
        // property) until a Measure pass.
        ScrollVector m_ComputedOffset;

        ScrollVector m_ArrangedOffset;
        
        // ViewportSize is in {pixels x items} (or vice-versa).
        wf::Size m_viewport;
        
        // Extent is the total number of children (logical dimension) or
        // physical size
        wf::Size m_extent;
        
        // Hold onto the maximum desired size to avoid re-laying out the parent
        // ScrollViewer.
        wf::Size m_MaxDesiredSize;
        
    private:
        // The ScrollViewer whose state is represented by this data.
        ctl::WeakRefPtr m_wrScrollOwner;
        
        // Scroll offset of content.  Positive corresponds to a visually upward
        // offset.  Set by methods like LineUp, PageDown, etc.
        // Private field to ensure the put_OffsetX/put_OffsetY setters are 
        // consistently used so no ScrollViewer.ViewChanging notification is missed.
        ScrollVector m_Offset;
        
    public:
        // Creates a new instance of the ScrollData class.
        ScrollData();
        
        // Creates a new instance of the ScrollData class.
        static _Check_return_ HRESULT Create(
            _Outptr_ ScrollData** ppScrollData);
        
        // Clears layout generated data.  It does not clear m_pScrollOwner,
        // because unless resetting due to a m_pScrollOwner change, we won't get
        // reattached.
        void ClearLayout();

        // Gets or sets the ScrollViewer whose state is represented by this data.
        _Check_return_ HRESULT get_ScrollOwner(
            _Outptr_ IScrollOwner** ppScrollOwner);
        _Check_return_ HRESULT put_ScrollOwner(
            _In_opt_ IScrollOwner* pScrollOwner);

        ScrollVector get_Offset()
        {
            return m_Offset;
        }

        DOUBLE get_OffsetX()
        {
            return m_Offset.X;
        }

        DOUBLE get_OffsetY()
        {
            return m_Offset.Y;
        }

        // Records a request for a new horizontal and vertical offset, 
        // and notifies the scroll owner of this request.
        _Check_return_ HRESULT put_Offset(
            _In_ const ScrollVector& offset);

        // Records a request for a new horizontal offset, 
        // and notifies the scroll owner of this request.
        _Check_return_ HRESULT put_OffsetX(
            _In_ DOUBLE offset);

        // Records a request for a new vertical offset, 
        // and notifies the scroll owner of this request.
        _Check_return_ HRESULT put_OffsetY(
            _In_ DOUBLE offset);
    };

    class OffsetMemento
    {
    public:
        OffsetMemento(
            _In_ xaml_controls::Orientation orientation,
            _In_ UINT realizedChildrenCount,
            _In_ UINT visualChildrenCount,
            _In_ ScrollData& scrollData);

        ~OffsetMemento();

        BOOLEAN Equals(_In_ OffsetMemento* pOffsetMemento);

        DOUBLE get_Delta();
        _Check_return_ HRESULT put_Delta(_In_ DOUBLE delta);

        DOUBLE get_UnusedDelta();
        _Check_return_ HRESULT put_UnusedDelta(_In_ DOUBLE delta);

        DOUBLE get_CurrentOffset();
        _Check_return_ HRESULT put_CurrentOffset(_In_ DOUBLE currentOffset);

        DOUBLE get_RequestedOffset();
        _Check_return_ HRESULT put_RequestedOffset(_In_ DOUBLE requestedOffset);

    private:
        DOUBLE* m_pStateDelta;
        DOUBLE* m_pStateUnusedDelta;
        DOUBLE* m_pStateCurrentOffset;
        DOUBLE* m_pStateRequestedOffset;
        xaml_controls::Orientation m_Orientation;
        UINT m_nRealizedItemsCount;
        UINT m_nVisualItemsCount;
        ScrollVector m_ComputedOffset;
        ScrollVector m_ArrangedOffset;
        wf::Size m_MaxDesiredSize;
    };
}

