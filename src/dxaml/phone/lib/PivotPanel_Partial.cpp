// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotPanel_Partial.h"
#include "Pivot_Partial.h"

#include <algorithm>

//#define PVTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define PVTRACE(...)

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{
PivotPanel::PivotPanel()
    : m_availableWidth(0.0)
    , m_headerHeight(0.0)
{}

PivotPanel::~PivotPanel()
{}

_Check_return_
HRESULT
PivotPanel::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IPanelFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IPanel> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(PivotPanelGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Panel).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<IPivotPanel*>(this)),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotPanel::SetParentPivot(_In_opt_ xaml_controls::IPivot* pivot)
{
    m_parentPivotWeakRef = nullptr;

    if (pivot != nullptr)
    {
        IFC_RETURN(wrl::ComPtr<xaml_controls::IPivot>(pivot).AsWeak(&m_parentPivotWeakRef));
    }
    return S_OK;
}

_Check_return_ HRESULT
PivotPanel::SetSectionWidth(_In_ FLOAT availableWidth)
{
    m_availableWidth = availableWidth;

    IFC_RETURN(InvalidateMeasure());

    // The snap points are based on window width, we know them
    // after a measure pass has occurred on the ScrollViewer's parent
    // and this function is called.
    IFC_RETURN(RaiseSnapPointsChangedEvents());

    return S_OK;
}

_Check_return_ HRESULT PivotPanel::InvalidateMeasure()
{
    wrl::ComPtr<xaml::IUIElement> spThisAsUE;
    IFC_RETURN(GetComposableBase().As(&spThisAsUE));
    IFC_RETURN(spThisAsUE->InvalidateMeasure());
    return S_OK;
}

_Check_return_ HRESULT
PivotPanel::ArrangeOverrideImpl(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* pReturnValue)
{
    wrl::ComPtr<xaml_controls::IPanel> spThisAsPanel;
    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildrenVect;

    if (m_parentPivotWeakRef)
    {
        wrl::ComPtr<xaml_controls::IPivot> parentPivot;
        IFC_RETURN(m_parentPivotWeakRef.As(&parentPivot));
        IFC_RETURN(static_cast<Pivot*>(parentPivot.Get())->InvalidateHeaderSecondaryContentRelationships());
    }

    IFC_RETURN(QueryInterface(
        __uuidof(xaml_controls::IPanel),
        &spThisAsPanel));
    IFC_RETURN(spThisAsPanel->get_Children(&spChildrenVect));

    UINT vectSize = 0;
    IFC_RETURN(spChildrenVect->get_Size(&vectSize));

    for (UINT childIdx = 0; childIdx < vectSize; childIdx++)
    {
        wrl::ComPtr<xaml::IUIElement> spChild;
        wrl::ComPtr<xaml::IFrameworkElement> spChildAsFE;
        bool didArrange = false;
        IFC_RETURN(spChildrenVect->GetAt(childIdx, &spChild));

        IGNOREHR(spChild.As(&spChildAsFE));

        if (spChildAsFE)
        {
            wrl_wrappers::HString childName;
            IFC_RETURN(spChildAsFE->get_Name(childName.ReleaseAndGetAddressOf()));

            // Why StringReference requries this const cast is beyond
            // my abilities to understand.

            if (childName == wrl_wrappers::HStringReference(Pivot::c_LayoutElementName))
            {
                wf::Rect finalArrangingRect = {};
                finalArrangingRect.Width = m_availableWidth;
                finalArrangingRect.Height = finalSize.Height;
                IFC_RETURN(spChild->Arrange(finalArrangingRect));
                didArrange = true;
            }
            else if (childName == wrl_wrappers::HStringReference(Pivot::c_PivotItemsPresenterName))
            {
                wf::Rect finalArrangingRect = {};
                finalArrangingRect.Y = m_headerHeight;
                finalArrangingRect.Height = finalSize.Height - m_headerHeight;
                // Lay all the elements out with the fixed width
                // they were measured with.
                finalArrangingRect.Width = m_availableWidth;

                IFC_RETURN(spChild->Arrange(finalArrangingRect));
                didArrange = true;
            }
            else if (childName == wrl_wrappers::HStringReference(Pivot::c_HeadersControlName))
            {
                wf::Rect finalArrangingRect = {};
                finalArrangingRect.Height = m_headerHeight;
                finalArrangingRect.Width = finalSize.Width;

                IFC_RETURN(spChild->Arrange(finalArrangingRect));
                didArrange = true;
            }
        }

        // Not sure what these other UI elements are doing in
        // our panel.
        if (!didArrange)
        {
            wf::Rect finalArrangingRect = {};
            finalArrangingRect.Width = finalSize.Width;
            finalArrangingRect.Height = finalSize.Height;
            IFC_RETURN(spChild->Arrange(finalArrangingRect));
        }
    }

    *pReturnValue = finalSize;

    return S_OK;
}

_Check_return_ HRESULT
PivotPanel::MeasureOverrideImpl(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* pDesiredSize)
{
    wrl::ComPtr<xaml_controls::IPanel> spThisAsPanel;
    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildrenVect;

    wrl::ComPtr<xaml::IUIElement> spLayoutElement;
    wrl::ComPtr<xaml::IUIElement> spItemsPresenter;
    wrl::ComPtr<Xaml::IUIElement> spHeaderControl;

    wf::Size desiredSize = {};

    IFC_RETURN(QueryInterface(
        __uuidof(xaml_controls::IPanel),
        &spThisAsPanel));
    IFC_RETURN(spThisAsPanel->get_Children(&spChildrenVect));

    UINT vectSize = 0;
    IFC_RETURN(spChildrenVect->get_Size(&vectSize));

    // First we iterate through the collection and find the
    // header to measure it first. We do this to store its
    // desired size for laying out the ItensPresenter properly.
    for (UINT childIdx = 0; childIdx < vectSize; childIdx++)
    {
        wrl::ComPtr<xaml::IUIElement> spChild;
        wrl::ComPtr<xaml::IFrameworkElement> spChildAsFE;
        BOOLEAN isExpectedElement = FALSE;
        IFC_RETURN(spChildrenVect->GetAt(childIdx, &spChild));

        IGNOREHR(spChild.As(&spChildAsFE));

        if (spChildAsFE)
        {
            wrl_wrappers::HString childName;
            IFC_RETURN(spChildAsFE->get_Name(childName.ReleaseAndGetAddressOf()));

            if (childName == wrl_wrappers::HStringReference(Pivot::c_LayoutElementName))
            {
                isExpectedElement = TRUE;
                spLayoutElement = spChild;
            }
            else if (childName == wrl_wrappers::HStringReference(Pivot::c_HeadersControlName))
            {
                isExpectedElement = TRUE;
                spHeaderControl = spChild;
            }
            else if (childName == wrl_wrappers::HStringReference(Pivot::c_PivotItemsPresenterName))
            {
                isExpectedElement = TRUE;
                spItemsPresenter = spChild;
            }
        }

        if (!isExpectedElement)
        {
            IFC_RETURN(spChild->Measure(availableSize));
        }
    }

    // In the Windows 10 template, we expect spLayoutElement which
    // will have spHeaderControl and spItemsPresenter as descendants.
    // Layout under spLayoutElement is flexible and follow xaml rules.
    IFCEXPECT_RETURN(spHeaderControl || spLayoutElement);
    IFCEXPECT_RETURN(spItemsPresenter || spLayoutElement);

    // In windows 10, we measure the layout element instead of measuring directly the
    // items presenter and the header panel.
    if (spLayoutElement)
    {
        const wf::Size constrainedAvailableSize = { m_availableWidth, availableSize.Height};
        IFC_RETURN(spLayoutElement->Measure(constrainedAvailableSize));
        IFC_RETURN(spLayoutElement->get_DesiredSize(&desiredSize));
    }

    if (spHeaderControl)
    {
        IFC_RETURN(spHeaderControl->Measure(availableSize));
        // Grab the desired height of the header so we can
        // lay everthing out in the arrange pass, subtracting
        // and offsetting the height from the item presenter panel.
        wf::Size desiredHeaderSize = {};
        IFC_RETURN(spHeaderControl->get_DesiredSize(&desiredHeaderSize));
        m_headerHeight = desiredHeaderSize.Height;
    }

    // Sets the available width to be the container's width to
    // lay out the PivotItems with a fixed size, even though
    // we're in a ScrollViewer that will measure with infinte
    // extents.
    if (spItemsPresenter)
    {
        wf::Size constrainedItemsPresenterSize = {};
        constrainedItemsPresenterSize.Height = availableSize.Height - m_headerHeight;
        constrainedItemsPresenterSize.Width = m_availableWidth;
        IFC_RETURN(spItemsPresenter->Measure(constrainedItemsPresenterSize));

        IFC_RETURN(spItemsPresenter->get_DesiredSize(&desiredSize));
    }

    // Set the desired height to be the size of the header and item
    // presenter to avoid returning infinite sizes.
    pDesiredSize->Height = desiredSize.Height + m_headerHeight;

    // Sets the desired width to be a large multiple of the
    // container's width to allow scrolling and align snap points.
    // If placed in an infinite panel we instead fall back on the
    // desired item presenter width.
    if (m_availableWidth == std::numeric_limits<FLOAT>::infinity())
    {
        m_availableWidth = desiredSize.Width;
    }

    unsigned panelMultiplier = GetPivotPanelMultiplier();
    if (m_parentPivotWeakRef)
    {
        wrl::ComPtr<xaml_controls::IPivot> parentPivot;
        IFC_RETURN(m_parentPivotWeakRef.As(&parentPivot));
        panelMultiplier = static_cast<Pivot*>(parentPivot.Get())->GetPivotPanelMultiplierImpl();
    }

    pDesiredSize->Width = m_availableWidth * panelMultiplier;

    return S_OK;
}

_Check_return_ HRESULT
PivotPanel::get_AreHorizontalSnapPointsRegularImpl(_Out_ boolean* pValue)
{
    *pValue = TRUE;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
PivotPanel::get_AreVerticalSnapPointsRegularImpl(_Out_ boolean* pValue)
{
    *pValue = TRUE;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
PivotPanel::GetIrregularSnapPointsImpl(_In_ xaml_controls::Orientation orientation, _In_ xaml_controls::Primitives::SnapPointsAlignment alignment, _Outptr_ wfc::IVectorView<FLOAT>** returnValue)
{
    // NOTE: This method should never be called, both
    // horizontal and vertical SnapPoints are ALWAYS regular.
    UNREFERENCED_PARAMETER(orientation);
    UNREFERENCED_PARAMETER(alignment);
    UNREFERENCED_PARAMETER(returnValue);
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT
PivotPanel::GetRegularSnapPointsImpl(_In_ xaml_controls::Orientation orientation, _In_ xaml_controls::Primitives::SnapPointsAlignment alignment, _Out_ FLOAT* offset, _Out_ FLOAT* returnValue)
{
    HRESULT hr = S_OK;
    // For now the PivotPanel will simply return a evenly spaced grid,
    // the vertical and horizontal snap points will be identical.
    UNREFERENCED_PARAMETER(orientation);
    UNREFERENCED_PARAMETER(alignment);

    IFCPTR(offset);
    IFCPTR(returnValue);

    *offset = m_availableWidth / 2;
    *returnValue = m_availableWidth;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotPanel::RaiseSnapPointsChangedEvents()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<IInspectable> spThisAsInspectable;

    IFC(QueryInterface(
        __uuidof(IInspectable),
        &spThisAsInspectable));

    IFC(m_VerticalSnapPointsChangedEventSource.InvokeAll(
        spThisAsInspectable.Get(),
        spThisAsInspectable.Get()));

    IFC(m_HorizontalSnapPointsChangedEventSource.InvokeAll(
        spThisAsInspectable.Get(),
        spThisAsInspectable.Get()));

Cleanup:
    RRETURN(hr);
}

}
} } } } XAML_ABI_NAMESPACE_END
