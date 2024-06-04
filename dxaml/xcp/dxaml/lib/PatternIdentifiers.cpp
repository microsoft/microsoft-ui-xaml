// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PatternIdentifiers.h"
#include "AutomationProperty.g.h"
#include "AutomationElementIdentifiers.h"

DirectUI::AnnotationPatternIdentifiers::AnnotationPatternIdentifiers()
{
}

DirectUI::AnnotationPatternIdentifiers::~AnnotationPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::AnnotationPatternIdentifiers::Close()
{
    m_spAnnotationTypeIdProperty.Reset();
    m_spAnnotationTypeNameProperty.Reset();
    m_spAuthorProperty.Reset();
    m_spDateTimeProperty.Reset();
    m_spTargetProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::AnnotationPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IAnnotationPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IAnnotationPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IAnnotationPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IAnnotationPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::AnnotationPatternIdentifiers::get_AnnotationTypeIdProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spAnnotationTypeIdProperty, AutomationPropertiesEnum::AnnotationTypeIdProperty));
    IFC_RETURN(m_spAnnotationTypeIdProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::AnnotationPatternIdentifiers::get_AnnotationTypeNameProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spAnnotationTypeNameProperty, AutomationPropertiesEnum::AnnotationTypeNameProperty));
    IFC_RETURN(m_spAnnotationTypeNameProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::AnnotationPatternIdentifiers::get_AuthorProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spAuthorProperty, AutomationPropertiesEnum::AuthorProperty));
    IFC_RETURN(m_spAuthorProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::AnnotationPatternIdentifiers::get_DateTimeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spDateTimeProperty, AutomationPropertiesEnum::DateTimeProperty));
    IFC_RETURN(m_spDateTimeProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::AnnotationPatternIdentifiers::get_TargetProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spTargetProperty, AutomationPropertiesEnum::TargetProperty));
    IFC_RETURN(m_spTargetProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::DockPatternIdentifiers::DockPatternIdentifiers()
{
}

DirectUI::DockPatternIdentifiers::~DockPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::DockPatternIdentifiers::Close()
{
    m_spDockPositionProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::DockPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IDockPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IDockPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IDockPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IDockPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::DockPatternIdentifiers::get_DockPositionProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spDockPositionProperty, AutomationPropertiesEnum::DockPositionProperty));
    IFC_RETURN(m_spDockPositionProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::DragPatternIdentifiers::DragPatternIdentifiers()
{
}

DirectUI::DragPatternIdentifiers::~DragPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::DragPatternIdentifiers::Close()
{
    m_spDropEffectProperty.Reset();
    m_spDropEffectsProperty.Reset();
    m_spGrabbedItemsProperty.Reset();
    m_spIsGrabbedProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::DragPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IDragPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IDragPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IDragPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IDragPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::DragPatternIdentifiers::get_DropEffectProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spDropEffectProperty, AutomationPropertiesEnum::DropEffectProperty));
    IFC_RETURN(m_spDropEffectProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::DragPatternIdentifiers::get_DropEffectsProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spDropEffectsProperty, AutomationPropertiesEnum::DropEffectsProperty));
    IFC_RETURN(m_spDropEffectsProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::DragPatternIdentifiers::get_GrabbedItemsProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spGrabbedItemsProperty, AutomationPropertiesEnum::GrabbedItemsProperty));
    IFC_RETURN(m_spGrabbedItemsProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::DragPatternIdentifiers::get_IsGrabbedProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsGrabbedProperty, AutomationPropertiesEnum::IsGrabbedProperty));
    IFC_RETURN(m_spIsGrabbedProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::DropTargetPatternIdentifiers::DropTargetPatternIdentifiers()
{
}

DirectUI::DropTargetPatternIdentifiers::~DropTargetPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::DropTargetPatternIdentifiers::Close()
{
    m_spDropTargetEffectProperty.Reset();
    m_spDropTargetEffectsProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::DropTargetPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IDropTargetPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IDropTargetPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IDropTargetPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IDropTargetPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::DropTargetPatternIdentifiers::get_DropTargetEffectProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spDropTargetEffectProperty, AutomationPropertiesEnum::DropTargetEffectProperty));
    IFC_RETURN(m_spDropTargetEffectProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::DropTargetPatternIdentifiers::get_DropTargetEffectsProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spDropTargetEffectsProperty, AutomationPropertiesEnum::DropTargetEffectsProperty));
    IFC_RETURN(m_spDropTargetEffectsProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::ExpandCollapsePatternIdentifiers::ExpandCollapsePatternIdentifiers()
{
}

DirectUI::ExpandCollapsePatternIdentifiers::~ExpandCollapsePatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::ExpandCollapsePatternIdentifiers::Close()
{
    m_spExpandCollapseStateProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::ExpandCollapsePatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IExpandCollapsePatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IExpandCollapsePatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IExpandCollapsePatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IExpandCollapsePatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::ExpandCollapsePatternIdentifiers::get_ExpandCollapseStateProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spExpandCollapseStateProperty, AutomationPropertiesEnum::ExpandCollapseStateProperty));
    IFC_RETURN(m_spExpandCollapseStateProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::GridItemPatternIdentifiers::GridItemPatternIdentifiers()
{
}

DirectUI::GridItemPatternIdentifiers::~GridItemPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::GridItemPatternIdentifiers::Close()
{
    m_spColumnProperty.Reset();
    m_spColumnSpanProperty.Reset();
    m_spContainingGridProperty.Reset();
    m_spRowProperty.Reset();
    m_spRowSpanProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::GridItemPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IGridItemPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IGridItemPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IGridItemPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IGridItemPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::GridItemPatternIdentifiers::get_ColumnProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);

    IFC_RETURN(AutomationProperty::EnsureProperty(m_spColumnProperty, AutomationPropertiesEnum::ColumnProperty));
    IFC_RETURN(m_spColumnProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::GridItemPatternIdentifiers::get_ColumnSpanProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spColumnSpanProperty, AutomationPropertiesEnum::ColumnSpanProperty));
    IFC_RETURN(m_spColumnSpanProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::GridItemPatternIdentifiers::get_ContainingGridProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spContainingGridProperty, AutomationPropertiesEnum::ContainingGridProperty));
    IFC_RETURN(m_spContainingGridProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::GridItemPatternIdentifiers::get_RowProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spRowProperty, AutomationPropertiesEnum::RowProperty));
    IFC_RETURN(m_spRowProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::GridItemPatternIdentifiers::get_RowSpanProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spRowSpanProperty, AutomationPropertiesEnum::RowSpanProperty));
    IFC_RETURN(m_spRowSpanProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::GridPatternIdentifiers::GridPatternIdentifiers()
{
}

DirectUI::GridPatternIdentifiers::~GridPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::GridPatternIdentifiers::Close()
{
    m_spColumnCountProperty.Reset();
    m_spRowCountProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::GridPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IGridPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IGridPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IGridPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IGridPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::GridPatternIdentifiers::get_ColumnCountProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spColumnCountProperty, AutomationPropertiesEnum::ColumnCountProperty));
    IFC_RETURN(m_spColumnCountProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::GridPatternIdentifiers::get_RowCountProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spRowCountProperty, AutomationPropertiesEnum::RowCountProperty));
    IFC_RETURN(m_spRowCountProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::MultipleViewPatternIdentifiers::MultipleViewPatternIdentifiers()
{
}

DirectUI::MultipleViewPatternIdentifiers::~MultipleViewPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::MultipleViewPatternIdentifiers::Close()
{
    m_spCurrentViewProperty.Reset();
    m_spSupportedViewsProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::MultipleViewPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IMultipleViewPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IMultipleViewPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IMultipleViewPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IMultipleViewPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::MultipleViewPatternIdentifiers::get_CurrentViewProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCurrentViewProperty, AutomationPropertiesEnum::CurrentViewProperty));
    IFC_RETURN(m_spCurrentViewProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::MultipleViewPatternIdentifiers::get_SupportedViewsProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spSupportedViewsProperty, AutomationPropertiesEnum::SupportedViewsProperty));
    IFC_RETURN(m_spSupportedViewsProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::RangeValuePatternIdentifiers::RangeValuePatternIdentifiers()
{
}

DirectUI::RangeValuePatternIdentifiers::~RangeValuePatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::RangeValuePatternIdentifiers::Close()
{
    m_spIsReadOnlyProperty.Reset();
    m_spLargeChangeProperty.Reset();
    m_spMaximumProperty.Reset();
    m_spMinimumProperty.Reset();
    m_spSmallChangeProperty.Reset();
    m_spValueProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::RangeValuePatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IRangeValuePatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IRangeValuePatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IRangeValuePatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IRangeValuePatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::RangeValuePatternIdentifiers::get_IsReadOnlyProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsReadOnlyProperty, AutomationPropertiesEnum::RangeValueIsReadOnlyProperty));
    IFC_RETURN(m_spIsReadOnlyProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::RangeValuePatternIdentifiers::get_LargeChangeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spLargeChangeProperty, AutomationPropertiesEnum::LargeChangeProperty));
    IFC_RETURN(m_spLargeChangeProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::RangeValuePatternIdentifiers::get_MaximumProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spMaximumProperty, AutomationPropertiesEnum::MaximumProperty));
    IFC_RETURN(m_spMaximumProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::RangeValuePatternIdentifiers::get_MinimumProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spMinimumProperty, AutomationPropertiesEnum::MinimumProperty));
    IFC_RETURN(m_spMinimumProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::RangeValuePatternIdentifiers::get_SmallChangeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spSmallChangeProperty, AutomationPropertiesEnum::SmallChangeProperty));
    IFC_RETURN(m_spSmallChangeProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::RangeValuePatternIdentifiers::get_ValueProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spValueProperty, AutomationPropertiesEnum::RangeValueValueProperty));
    IFC_RETURN(m_spValueProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::ScrollPatternIdentifiers::ScrollPatternIdentifiers()
{
}

DirectUI::ScrollPatternIdentifiers::~ScrollPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::ScrollPatternIdentifiers::Close()
{
    m_spHorizontallyScrollableProperty.Reset();
    m_spHorizontalScrollPercentProperty.Reset();
    m_spHorizontalViewSizeProperty.Reset();
    m_spVerticallyScrollableProperty.Reset();
    m_spVerticalScrollPercentProperty.Reset();
    m_spVerticalViewSizeProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::ScrollPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IScrollPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IScrollPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IScrollPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IScrollPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::ScrollPatternIdentifiers::get_HorizontallyScrollableProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spHorizontallyScrollableProperty, AutomationPropertiesEnum::HorizontallyScrollableProperty));
    IFC_RETURN(m_spHorizontallyScrollableProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::ScrollPatternIdentifiers::get_HorizontalScrollPercentProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spHorizontalScrollPercentProperty, AutomationPropertiesEnum::HorizontalScrollPercentProperty));
    IFC_RETURN(m_spHorizontalScrollPercentProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::ScrollPatternIdentifiers::get_HorizontalViewSizeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spHorizontalViewSizeProperty, AutomationPropertiesEnum::HorizontalViewSizeProperty));
    IFC_RETURN(m_spHorizontalViewSizeProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::ScrollPatternIdentifiers::get_NoScroll(_Out_ DOUBLE* pValue)
{
    IFCPTR_RETURN(pValue);
    
    *pValue = -1.0;

    return S_OK;
}
HRESULT DirectUI::ScrollPatternIdentifiers::get_VerticallyScrollableProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spVerticallyScrollableProperty, AutomationPropertiesEnum::VerticallyScrollableProperty));
    IFC_RETURN(m_spVerticallyScrollableProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::ScrollPatternIdentifiers::get_VerticalScrollPercentProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spVerticalScrollPercentProperty, AutomationPropertiesEnum::VerticalScrollPercentProperty));
    IFC_RETURN(m_spVerticalScrollPercentProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::ScrollPatternIdentifiers::get_VerticalViewSizeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spVerticalViewSizeProperty, AutomationPropertiesEnum::VerticalViewSizeProperty));
    IFC_RETURN(m_spVerticalViewSizeProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::SelectionItemPatternIdentifiers::SelectionItemPatternIdentifiers()
{
}

DirectUI::SelectionItemPatternIdentifiers::~SelectionItemPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::SelectionItemPatternIdentifiers::Close()
{
    m_spIsSelectedProperty.Reset();
    m_spSelectionContainerProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::SelectionItemPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ISelectionItemPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ISelectionItemPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ISelectionItemPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::ISelectionItemPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::SelectionItemPatternIdentifiers::get_IsSelectedProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsSelectedProperty, AutomationPropertiesEnum::IsSelectedProperty));
    IFC_RETURN(m_spIsSelectedProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::SelectionItemPatternIdentifiers::get_SelectionContainerProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spSelectionContainerProperty, AutomationPropertiesEnum::SelectionContainerProperty));
    IFC_RETURN(m_spSelectionContainerProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::SelectionPatternIdentifiers::SelectionPatternIdentifiers()
{
}

DirectUI::SelectionPatternIdentifiers::~SelectionPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::SelectionPatternIdentifiers::Close()
{
    m_spCanSelectMultipleProperty.Reset();
    m_spIsSelectionRequiredProperty.Reset();
    m_spSelectionProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::SelectionPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ISelectionPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ISelectionPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ISelectionPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::ISelectionPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::SelectionPatternIdentifiers::get_CanSelectMultipleProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCanSelectMultipleProperty, AutomationPropertiesEnum::CanSelectMultipleProperty));
    IFC_RETURN(m_spCanSelectMultipleProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::SelectionPatternIdentifiers::get_IsSelectionRequiredProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsSelectionRequiredProperty, AutomationPropertiesEnum::IsSelectionRequiredProperty));
    IFC_RETURN(m_spIsSelectionRequiredProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::SelectionPatternIdentifiers::get_SelectionProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spSelectionProperty, AutomationPropertiesEnum::SelectionProperty));
    IFC_RETURN(m_spSelectionProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::TableItemPatternIdentifiers::TableItemPatternIdentifiers()
{
}

DirectUI::TableItemPatternIdentifiers::~TableItemPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::TableItemPatternIdentifiers::Close()
{
    m_spColumnHeaderItemsProperty.Reset();
    m_spRowHeaderItemsProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::TableItemPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITableItemPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ITableItemPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITableItemPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::ITableItemPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::TableItemPatternIdentifiers::get_ColumnHeaderItemsProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spColumnHeaderItemsProperty, AutomationPropertiesEnum::ColumnHeaderItemsProperty));
    IFC_RETURN(m_spColumnHeaderItemsProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::TableItemPatternIdentifiers::get_RowHeaderItemsProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spRowHeaderItemsProperty, AutomationPropertiesEnum::RowHeaderItemsProperty));
    IFC_RETURN(m_spRowHeaderItemsProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::TablePatternIdentifiers::TablePatternIdentifiers()
{
}

DirectUI::TablePatternIdentifiers::~TablePatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::TablePatternIdentifiers::Close()
{
    m_spColumnHeadersProperty.Reset();
    m_spRowHeadersProperty.Reset();
    m_spRowOrColumnMajorProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::TablePatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITablePatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ITablePatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITablePatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::ITablePatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::TablePatternIdentifiers::get_ColumnHeadersProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spColumnHeadersProperty, AutomationPropertiesEnum::ColumnHeadersProperty));
    IFC_RETURN(m_spColumnHeadersProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::TablePatternIdentifiers::get_RowHeadersProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spRowHeadersProperty, AutomationPropertiesEnum::RowHeadersProperty));
    IFC_RETURN(m_spRowHeadersProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::TablePatternIdentifiers::get_RowOrColumnMajorProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spRowOrColumnMajorProperty, AutomationPropertiesEnum::RowOrColumnMajorProperty));
    IFC_RETURN(m_spRowOrColumnMajorProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::TogglePatternIdentifiers::TogglePatternIdentifiers()
{
}

DirectUI::TogglePatternIdentifiers::~TogglePatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::TogglePatternIdentifiers::Close()
{
    m_spToggleStateProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::TogglePatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITogglePatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ITogglePatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITogglePatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::ITogglePatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::TogglePatternIdentifiers::get_ToggleStateProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spToggleStateProperty, AutomationPropertiesEnum::ToggleStateProperty));
    IFC_RETURN(m_spToggleStateProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::TransformPatternIdentifiers::TransformPatternIdentifiers()
{
}

DirectUI::TransformPatternIdentifiers::~TransformPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::TransformPatternIdentifiers::Close()
{
    m_spCanMoveProperty.Reset();
    m_spCanResizeProperty.Reset();
    m_spCanRotateProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::TransformPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITransformPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ITransformPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITransformPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::ITransformPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::TransformPatternIdentifiers::get_CanMoveProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCanMoveProperty, AutomationPropertiesEnum::CanMoveProperty));
    IFC_RETURN(m_spCanMoveProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::TransformPatternIdentifiers::get_CanResizeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCanResizeProperty, AutomationPropertiesEnum::CanResizeProperty));
    IFC_RETURN(m_spCanResizeProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::TransformPatternIdentifiers::get_CanRotateProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCanRotateProperty, AutomationPropertiesEnum::CanRotateProperty));
    IFC_RETURN(m_spCanRotateProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::ValuePatternIdentifiers::ValuePatternIdentifiers()
{
}

DirectUI::ValuePatternIdentifiers::~ValuePatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::ValuePatternIdentifiers::Close()
{
    m_spIsReadOnlyProperty.Reset();
    m_spValueProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::ValuePatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IValuePatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IValuePatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IValuePatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IValuePatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::ValuePatternIdentifiers::get_IsReadOnlyProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsReadOnlyProperty, AutomationPropertiesEnum::ValueIsReadOnlyProperty));
    IFC_RETURN(m_spIsReadOnlyProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::ValuePatternIdentifiers::get_ValueProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spValueProperty, AutomationPropertiesEnum::ValueValueProperty));
    IFC_RETURN(m_spValueProperty.CopyTo(ppValue));

    return S_OK;
}

DirectUI::WindowPatternIdentifiers::WindowPatternIdentifiers()
{
}

DirectUI::WindowPatternIdentifiers::~WindowPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::WindowPatternIdentifiers::Close()
{
    m_spCanMaximizeProperty.Reset();
    m_spCanMinimizeProperty.Reset();
    m_spIsModalProperty.Reset();
    m_spIsTopmostProperty.Reset();
    m_spWindowInteractionStateProperty.Reset();
    m_spWindowVisualStateProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::WindowPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IWindowPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IWindowPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IWindowPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IWindowPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::WindowPatternIdentifiers::get_CanMaximizeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCanMaximizeProperty, AutomationPropertiesEnum::CanMaximizeProperty));
    IFC_RETURN(m_spCanMaximizeProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::WindowPatternIdentifiers::get_CanMinimizeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCanMinimizeProperty, AutomationPropertiesEnum::CanMinimizeProperty));
    IFC_RETURN(m_spCanMinimizeProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::WindowPatternIdentifiers::get_IsModalProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsModalProperty, AutomationPropertiesEnum::IsModalProperty));
    IFC_RETURN(m_spIsModalProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::WindowPatternIdentifiers::get_IsTopmostProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsTopmostProperty, AutomationPropertiesEnum::IsTopmostProperty));
    IFC_RETURN(m_spIsTopmostProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::WindowPatternIdentifiers::get_WindowInteractionStateProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spWindowInteractionStateProperty, AutomationPropertiesEnum::WindowInteractionStateProperty));
    IFC_RETURN(m_spWindowInteractionStateProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::WindowPatternIdentifiers::get_WindowVisualStateProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spWindowVisualStateProperty, AutomationPropertiesEnum::WindowVisualStateProperty));
    IFC_RETURN(m_spWindowVisualStateProperty.CopyTo(ppValue));

    return S_OK;
}


DirectUI::TransformPattern2Identifiers::TransformPattern2Identifiers()
{
}

DirectUI::TransformPattern2Identifiers::~TransformPattern2Identifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::TransformPattern2Identifiers::Close()
{
    m_spCanZoomProperty.Reset();
    m_spZoomLevelProperty.Reset();
    m_spMaxZoomProperty.Reset();
    m_spMinZoomProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::TransformPattern2Identifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITransformPattern2IdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ITransformPattern2IdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ITransformPattern2Identifiers)))
    {
        *ppObject = static_cast<xaml_automation::ITransformPattern2Identifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::TransformPattern2Identifiers::get_CanZoomProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spCanZoomProperty, AutomationPropertiesEnum::CanZoomProperty));
    IFC_RETURN(m_spCanZoomProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::TransformPattern2Identifiers::get_ZoomLevelProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spZoomLevelProperty, AutomationPropertiesEnum::ZoomLevelProperty));
    IFC_RETURN(m_spZoomLevelProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::TransformPattern2Identifiers::get_MaxZoomProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spMaxZoomProperty, AutomationPropertiesEnum::MaxZoomProperty));
    IFC_RETURN(m_spMaxZoomProperty.CopyTo(ppValue));

    return S_OK;
}

HRESULT DirectUI::TransformPattern2Identifiers::get_MinZoomProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spMinZoomProperty, AutomationPropertiesEnum::MinZoomProperty));
    IFC_RETURN(m_spMinZoomProperty.CopyTo(ppValue));

    return S_OK;
}


DirectUI::SpreadsheetItemPatternIdentifiers::SpreadsheetItemPatternIdentifiers()
{
}

DirectUI::SpreadsheetItemPatternIdentifiers::~SpreadsheetItemPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::SpreadsheetItemPatternIdentifiers::Close()
{
    m_spFormulaProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::SpreadsheetItemPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ISpreadsheetItemPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::ISpreadsheetItemPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::ISpreadsheetItemPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::ISpreadsheetItemPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::SpreadsheetItemPatternIdentifiers::get_FormulaProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spFormulaProperty, AutomationPropertiesEnum::FormulaProperty));
    IFC_RETURN(m_spFormulaProperty.CopyTo(ppValue));

    return S_OK;
}


DirectUI::StylesPatternIdentifiers::StylesPatternIdentifiers()
{
}

DirectUI::StylesPatternIdentifiers::~StylesPatternIdentifiers()
{
}

#if XCP_MONITOR
HRESULT DirectUI::StylesPatternIdentifiers::Close()
{
    m_spExtendedPropertiesProperty.Reset();
    m_spFillColorProperty.Reset();
    m_spFillPatternColorProperty.Reset();
    m_spFillPatternStyleProperty.Reset();
    m_spShapeProperty.Reset();
    m_spStyleIdProperty.Reset();
    m_spStyleNameProperty.Reset();
    return S_OK;
}
#endif

HRESULT DirectUI::StylesPatternIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IStylesPatternIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IStylesPatternIdentifiersStatics*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IStylesPatternIdentifiers)))
    {
        *ppObject = static_cast<xaml_automation::IStylesPatternIdentifiers*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::StylesPatternIdentifiers::get_ExtendedPropertiesProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spExtendedPropertiesProperty, AutomationPropertiesEnum::ExtendedPropertiesProperty));
    IFC_RETURN(m_spExtendedPropertiesProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::StylesPatternIdentifiers::get_FillColorProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spFillColorProperty, AutomationPropertiesEnum::FillColorProperty));
    IFC_RETURN(m_spFillColorProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::StylesPatternIdentifiers::get_FillPatternColorProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spFillPatternColorProperty, AutomationPropertiesEnum::FillPatternColorProperty));
    IFC_RETURN(m_spFillPatternColorProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::StylesPatternIdentifiers::get_FillPatternStyleProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spFillPatternStyleProperty, AutomationPropertiesEnum::FillPatternStyleProperty));
    IFC_RETURN(m_spFillPatternStyleProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::StylesPatternIdentifiers::get_ShapeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spShapeProperty, AutomationPropertiesEnum::ShapeProperty));
    IFC_RETURN(m_spShapeProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::StylesPatternIdentifiers::get_StyleIdProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spStyleIdProperty, AutomationPropertiesEnum::StyleIdProperty));
    IFC_RETURN(m_spStyleIdProperty.CopyTo(ppValue));

    return S_OK;
}
HRESULT DirectUI::StylesPatternIdentifiers::get_StyleNameProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spStyleNameProperty, AutomationPropertiesEnum::StyleNameProperty));
    IFC_RETURN(m_spStyleNameProperty.CopyTo(ppValue));

    return S_OK;
}


namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_AnnotationPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<AnnotationPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_AutomationElementIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<AutomationElementIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_DockPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<DockPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_DragPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<DragPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_DropTargetPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<DropTargetPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_ExpandCollapsePatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<ExpandCollapsePatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_GridItemPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<GridItemPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_GridPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<GridPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_MultipleViewPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<MultipleViewPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_RangeValuePatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<RangeValuePatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_ScrollPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<ScrollPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_SelectionItemPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<SelectionItemPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_SelectionPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<SelectionPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_SpreadsheetItemPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<SpreadsheetItemPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_StylesPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<StylesPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_TableItemPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<TableItemPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_TablePatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<TablePatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_TogglePatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<TogglePatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_TransformPattern2Identifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<TransformPattern2Identifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_TransformPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<TransformPatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_ValuePatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<ValuePatternIdentifiers>::CreateActivationFactory());
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_WindowPatternIdentifiers()
    {
        RRETURN(ctl::ActivationFactoryCreator<WindowPatternIdentifiers>::CreateActivationFactory());
    }
}
