// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class AnnotationPatternIdentifiers : 
        public xaml_automation::IAnnotationPatternIdentifiers,
        public xaml_automation::IAnnotationPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {

        BEGIN_INTERFACE_MAP(AnnotationPatternIdentifiers, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(AnnotationPatternIdentifiers,  xaml_automation::IAnnotationPatternIdentifiers)
            INTERFACE_ENTRY(AnnotationPatternIdentifiers,  xaml_automation::IAnnotationPatternIdentifiersStatics)
        END_INTERFACE_MAP(AnnotationPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_AnnotationTypeIdProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_AnnotationTypeNameProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_AuthorProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_DateTimeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_TargetProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            AnnotationPatternIdentifiers();
            ~AnnotationPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spAnnotationTypeIdProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spAnnotationTypeNameProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spAuthorProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spDateTimeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spTargetProperty;
    };

    class DockPatternIdentifiers : 
        public xaml_automation::IDockPatternIdentifiers,
        public xaml_automation::IDockPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {

        BEGIN_INTERFACE_MAP(DockPatternIdentifiers, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(DockPatternIdentifiers,  xaml_automation::IDockPatternIdentifiers)
            INTERFACE_ENTRY(DockPatternIdentifiers,  xaml_automation::IDockPatternIdentifiersStatics)
        END_INTERFACE_MAP(DockPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_DockPositionProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            DockPatternIdentifiers();
            ~DockPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spDockPositionProperty;
    };
    
    class DragPatternIdentifiers : 
        public xaml_automation::IDragPatternIdentifiers,
        public xaml_automation::IDragPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {

        BEGIN_INTERFACE_MAP(DragPatternIdentifiers, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(DragPatternIdentifiers,  xaml_automation::IDragPatternIdentifiers)
            INTERFACE_ENTRY(DragPatternIdentifiers,  xaml_automation::IDragPatternIdentifiersStatics)
        END_INTERFACE_MAP(DragPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_DropEffectProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_DropEffectsProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_GrabbedItemsProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsGrabbedProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            
            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif
        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            DragPatternIdentifiers();
            ~DragPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spDropEffectProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spDropEffectsProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spGrabbedItemsProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsGrabbedProperty;
    };

    class DropTargetPatternIdentifiers : 
        public xaml_automation::IDropTargetPatternIdentifiers,
        public xaml_automation::IDropTargetPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {

        BEGIN_INTERFACE_MAP(DropTargetPatternIdentifiers, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(DropTargetPatternIdentifiers,  xaml_automation::IDropTargetPatternIdentifiers)
            INTERFACE_ENTRY(DropTargetPatternIdentifiers,  xaml_automation::IDropTargetPatternIdentifiersStatics)
        END_INTERFACE_MAP(DropTargetPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_DropTargetEffectProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_DropTargetEffectsProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif
        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            DropTargetPatternIdentifiers();
            ~DropTargetPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spDropTargetEffectProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spDropTargetEffectsProperty;
    };

    class ExpandCollapsePatternIdentifiers : 
        public xaml_automation::IExpandCollapsePatternIdentifiers,
        public xaml_automation::IExpandCollapsePatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(ExpandCollapsePatternIdentifiers, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(ExpandCollapsePatternIdentifiers,  xaml_automation::IExpandCollapsePatternIdentifiers)
            INTERFACE_ENTRY(ExpandCollapsePatternIdentifiers,  xaml_automation::IExpandCollapsePatternIdentifiersStatics)
        END_INTERFACE_MAP(ExpandCollapsePatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_ExpandCollapseStateProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            ExpandCollapsePatternIdentifiers();
            ~ExpandCollapsePatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spExpandCollapseStateProperty;
    };

    class GridItemPatternIdentifiers: 
        public xaml_automation::IGridItemPatternIdentifiers,
        public xaml_automation::IGridItemPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(GridItemPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(GridItemPatternIdentifiers,  xaml_automation::IGridItemPatternIdentifiers)
            INTERFACE_ENTRY(GridItemPatternIdentifiers,  xaml_automation::IGridItemPatternIdentifiersStatics)
        END_INTERFACE_MAP(GridItemPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_ColumnProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ColumnSpanProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ContainingGridProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_RowProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_RowSpanProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            GridItemPatternIdentifiers();
            ~GridItemPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spColumnProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spColumnSpanProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spContainingGridProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spRowProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spRowSpanProperty;
    };
 
    class GridPatternIdentifiers: 
        public xaml_automation::IGridPatternIdentifiers,
        public xaml_automation::IGridPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(GridPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(GridPatternIdentifiers,  xaml_automation::IGridPatternIdentifiers)
            INTERFACE_ENTRY(GridPatternIdentifiers,  xaml_automation::IGridPatternIdentifiersStatics)
        END_INTERFACE_MAP(GridPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_ColumnCountProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_RowCountProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            GridPatternIdentifiers();
            ~GridPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spColumnCountProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spRowCountProperty;
    };

    class MultipleViewPatternIdentifiers: 
        public xaml_automation::IMultipleViewPatternIdentifiers,
        public xaml_automation::IMultipleViewPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(MultipleViewPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(MultipleViewPatternIdentifiers,  xaml_automation::IMultipleViewPatternIdentifiers)
            INTERFACE_ENTRY(MultipleViewPatternIdentifiers,  xaml_automation::IMultipleViewPatternIdentifiersStatics)
        END_INTERFACE_MAP(MultipleViewPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_CurrentViewProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_SupportedViewsProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            MultipleViewPatternIdentifiers();
            ~MultipleViewPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCurrentViewProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spSupportedViewsProperty;
    };

    class RangeValuePatternIdentifiers: 
        public xaml_automation::IRangeValuePatternIdentifiers,
        public xaml_automation::IRangeValuePatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(RangeValuePatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(RangeValuePatternIdentifiers,  xaml_automation::IRangeValuePatternIdentifiers)
            INTERFACE_ENTRY(RangeValuePatternIdentifiers,  xaml_automation::IRangeValuePatternIdentifiersStatics)
        END_INTERFACE_MAP(RangeValuePatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_IsReadOnlyProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_LargeChangeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_MaximumProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_MinimumProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_SmallChangeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ValueProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            RangeValuePatternIdentifiers();
            ~RangeValuePatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsReadOnlyProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spLargeChangeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spMaximumProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spMinimumProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spSmallChangeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spValueProperty;
    };

    class ScrollPatternIdentifiers: 
        public xaml_automation::IScrollPatternIdentifiers,
        public xaml_automation::IScrollPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(ScrollPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(ScrollPatternIdentifiers,  xaml_automation::IScrollPatternIdentifiers)
            INTERFACE_ENTRY(ScrollPatternIdentifiers,  xaml_automation::IScrollPatternIdentifiersStatics)
        END_INTERFACE_MAP(ScrollPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_HorizontallyScrollableProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_HorizontalScrollPercentProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_HorizontalViewSizeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_NoScroll)(_Out_ DOUBLE* pValue);
            IFACEMETHOD(get_VerticallyScrollableProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_VerticalScrollPercentProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_VerticalViewSizeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            ScrollPatternIdentifiers();
            ~ScrollPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spHorizontallyScrollableProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spHorizontalScrollPercentProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spHorizontalViewSizeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spVerticallyScrollableProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spVerticalScrollPercentProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spVerticalViewSizeProperty;
    };

    class SelectionItemPatternIdentifiers: 
        public xaml_automation::ISelectionItemPatternIdentifiers,
        public xaml_automation::ISelectionItemPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(SelectionItemPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(SelectionItemPatternIdentifiers,  xaml_automation::ISelectionItemPatternIdentifiers)
            INTERFACE_ENTRY(SelectionItemPatternIdentifiers,  xaml_automation::ISelectionItemPatternIdentifiersStatics)
        END_INTERFACE_MAP(SelectionItemPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_IsSelectedProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_SelectionContainerProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            SelectionItemPatternIdentifiers();
            ~SelectionItemPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsSelectedProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spSelectionContainerProperty;
    };

    class SelectionPatternIdentifiers: 
        public xaml_automation::ISelectionPatternIdentifiers,
        public xaml_automation::ISelectionPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(SelectionPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(SelectionPatternIdentifiers,  xaml_automation::ISelectionPatternIdentifiers)
            INTERFACE_ENTRY(SelectionPatternIdentifiers,  xaml_automation::ISelectionPatternIdentifiersStatics)
        END_INTERFACE_MAP(SelectionPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_CanSelectMultipleProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsSelectionRequiredProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_SelectionProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            SelectionPatternIdentifiers();
            ~SelectionPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCanSelectMultipleProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsSelectionRequiredProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spSelectionProperty;
    };

    class TableItemPatternIdentifiers: 
        public xaml_automation::ITableItemPatternIdentifiers,
        public xaml_automation::ITableItemPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(TableItemPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(TableItemPatternIdentifiers,  xaml_automation::ITableItemPatternIdentifiers)
            INTERFACE_ENTRY(TableItemPatternIdentifiers,  xaml_automation::ITableItemPatternIdentifiersStatics)
        END_INTERFACE_MAP(TableItemPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_ColumnHeaderItemsProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_RowHeaderItemsProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            TableItemPatternIdentifiers();
            ~TableItemPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spColumnHeaderItemsProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spRowHeaderItemsProperty;
    };

    class TablePatternIdentifiers: 
        public xaml_automation::ITablePatternIdentifiers,
        public xaml_automation::ITablePatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(TablePatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(TablePatternIdentifiers,  xaml_automation::ITablePatternIdentifiers)
            INTERFACE_ENTRY(TablePatternIdentifiers,  xaml_automation::ITablePatternIdentifiersStatics)
        END_INTERFACE_MAP(TablePatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_ColumnHeadersProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_RowHeadersProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_RowOrColumnMajorProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            TablePatternIdentifiers();
            ~TablePatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spColumnHeadersProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spRowHeadersProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spRowOrColumnMajorProperty;
    };

    class TogglePatternIdentifiers: 
        public xaml_automation::ITogglePatternIdentifiers,
        public xaml_automation::ITogglePatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(TogglePatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(TogglePatternIdentifiers,  xaml_automation::ITogglePatternIdentifiers)
            INTERFACE_ENTRY(TogglePatternIdentifiers,  xaml_automation::ITogglePatternIdentifiersStatics)
        END_INTERFACE_MAP(TogglePatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_ToggleStateProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            TogglePatternIdentifiers();
            ~TogglePatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spToggleStateProperty;
    };

    class TransformPatternIdentifiers: 
        public xaml_automation::ITransformPatternIdentifiers,
        public xaml_automation::ITransformPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(TransformPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(TransformPatternIdentifiers,  xaml_automation::ITransformPatternIdentifiers)
            INTERFACE_ENTRY(TransformPatternIdentifiers,  xaml_automation::ITransformPatternIdentifiersStatics)
        END_INTERFACE_MAP(TransformPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_CanMoveProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_CanResizeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_CanRotateProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            TransformPatternIdentifiers();
            ~TransformPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCanMoveProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCanResizeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCanRotateProperty;
    };

    class ValuePatternIdentifiers: 
        public xaml_automation::IValuePatternIdentifiers,
        public xaml_automation::IValuePatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(ValuePatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(ValuePatternIdentifiers,  xaml_automation::IValuePatternIdentifiers)
            INTERFACE_ENTRY(ValuePatternIdentifiers,  xaml_automation::IValuePatternIdentifiersStatics)
        END_INTERFACE_MAP(ValuePatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_IsReadOnlyProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ValueProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            ValuePatternIdentifiers();
            ~ValuePatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsReadOnlyProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spValueProperty;
    };

    class WindowPatternIdentifiers: 
        public xaml_automation::IWindowPatternIdentifiers,
        public xaml_automation::IWindowPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(WindowPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(WindowPatternIdentifiers,  xaml_automation::IWindowPatternIdentifiers)
            INTERFACE_ENTRY(WindowPatternIdentifiers,  xaml_automation::IWindowPatternIdentifiersStatics)
        END_INTERFACE_MAP(WindowPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_CanMaximizeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_CanMinimizeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsModalProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsTopmostProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_WindowInteractionStateProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_WindowVisualStateProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            WindowPatternIdentifiers();
            ~WindowPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCanMaximizeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCanMinimizeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsModalProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsTopmostProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spWindowInteractionStateProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spWindowVisualStateProperty;
    };

    class TransformPattern2Identifiers: 
        public xaml_automation::ITransformPattern2Identifiers,
        public xaml_automation::ITransformPattern2IdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(TransformPattern2Identifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(TransformPattern2Identifiers,  xaml_automation::ITransformPattern2Identifiers)
            INTERFACE_ENTRY(TransformPattern2Identifiers,  xaml_automation::ITransformPattern2IdentifiersStatics)
        END_INTERFACE_MAP(TransformPattern2Identifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_CanZoomProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ZoomLevelProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_MaxZoomProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_MinZoomProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            TransformPattern2Identifiers();
            ~TransformPattern2Identifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spCanZoomProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spZoomLevelProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spMaxZoomProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spMinZoomProperty;
    };

    class SpreadsheetItemPatternIdentifiers: 
        public xaml_automation::ISpreadsheetItemPatternIdentifiers,
        public xaml_automation::ISpreadsheetItemPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(SpreadsheetItemPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(SpreadsheetItemPatternIdentifiers,  xaml_automation::ISpreadsheetItemPatternIdentifiers)
            INTERFACE_ENTRY(SpreadsheetItemPatternIdentifiers,  xaml_automation::ISpreadsheetItemPatternIdentifiersStatics)
        END_INTERFACE_MAP(SpreadsheetItemPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_FormulaProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            SpreadsheetItemPatternIdentifiers();
            ~SpreadsheetItemPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spFormulaProperty;
    };

    class StylesPatternIdentifiers: 
        public xaml_automation::IStylesPatternIdentifiers,
        public xaml_automation::IStylesPatternIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(StylesPatternIdentifiers,ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(StylesPatternIdentifiers,  xaml_automation::IStylesPatternIdentifiers)
            INTERFACE_ENTRY(StylesPatternIdentifiers,  xaml_automation::IStylesPatternIdentifiersStatics)
        END_INTERFACE_MAP(StylesPatternIdentifiers, ctl::AbstractActivationFactory)
            
        public:
            // Properties.
            IFACEMETHOD(get_ExtendedPropertiesProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_FillColorProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_FillPatternColorProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_FillPatternStyleProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ShapeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_StyleIdProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_StyleNameProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);

            // Override Close in debug builds so we don't get false positives
#if XCP_MONITOR
            IFACEMETHOD(Close)();
#endif

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            StylesPatternIdentifiers();
            ~StylesPatternIdentifiers() override;

        private:
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spExtendedPropertiesProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spFillColorProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spFillPatternColorProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spFillPatternStyleProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spShapeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spStyleIdProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spStyleNameProperty;
    };  
}
