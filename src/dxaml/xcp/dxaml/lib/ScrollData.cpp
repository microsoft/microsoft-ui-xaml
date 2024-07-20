// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollData.h"
#include "IScrollOwner.h"

using namespace DirectUI;

// Creates a new instance of the ScrollData class.
ScrollData::ScrollData()
{
    ClearLayout();
}

// Creates a new instance of the ScrollData class.
_Check_return_ HRESULT ScrollData::Create(
    _Outptr_ ScrollData** ppScrollData)
{
    HRESULT hr = S_OK;
    ScrollData* pScrollData = NULL;
    
    pScrollData = new ScrollData();

    *ppScrollData = pScrollData;
    RRETURN(hr);//RRETURN_REMOVAL
}

// Clears layout generated data.  It does not clear m_wrScrollOwner, because
// unless resetting due to a m_wrScrollOwner change, we won't get reattached.
void ScrollData::ClearLayout()
{
    wf::Size emptySize = {};
    ScrollVector emptyVector = {};
    m_viewport = m_extent = m_MaxDesiredSize = emptySize;
    m_Offset = m_MinOffset = m_ComputedOffset = m_ArrangedOffset = emptyVector;
}

// Gets or sets the ScrollViewer whose state is represented by this data.
_Check_return_ HRESULT ScrollData::get_ScrollOwner(
    _Outptr_ IScrollOwner** ppScrollOwner)
{
    RRETURN(m_wrScrollOwner.CopyTo(ppScrollOwner));
}

_Check_return_ HRESULT ScrollData::put_ScrollOwner(
    _In_opt_ IScrollOwner* pScrollOwner)
{
    RRETURN(ctl::AsWeak(pScrollOwner, &m_wrScrollOwner));
}

// Records a request for a new horizontal and vertical offset, 
// and notifies the scroll owner of this request.
_Check_return_ HRESULT ScrollData::put_Offset(
    _In_ const ScrollVector& offset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_wrScrollOwner.As(&spScrollOwner));
    if (spScrollOwner)
    {
        if (!DoubleUtil::AreClose(offset.X, m_Offset.X))
        {
            IFC(spScrollOwner->NotifyHorizontalOffsetChanging(offset.X, offset.Y));
        }
    
        if (!DoubleUtil::AreClose(offset.Y, m_Offset.Y))
        {
            IFC(spScrollOwner->NotifyVerticalOffsetChanging(offset.X, offset.Y));
        }
    }

    m_Offset = offset;

Cleanup:
    RRETURN(hr);
}

// Records a request for a new horizontal offset, 
// and notifies the scroll owner of this request.
_Check_return_ HRESULT ScrollData::put_OffsetX(
    _In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_wrScrollOwner.As(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->NotifyHorizontalOffsetChanging(offset, m_Offset.Y));
    }

    m_Offset.X = offset;

Cleanup:
    RRETURN(hr);
}

// Records a request for a new vertical offset, 
// and notifies the scroll owner of this request.
_Check_return_ HRESULT ScrollData::put_OffsetY(
    _In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_wrScrollOwner.As(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->NotifyVerticalOffsetChanging(m_Offset.X, offset));
    }

    m_Offset.Y = offset;

Cleanup:
    RRETURN(hr);
}

OffsetMemento::OffsetMemento(
    _In_ xaml_controls::Orientation orientation,
    _In_ UINT realizedChildrenCount,
    _In_ UINT visualChildrenCount,
    _In_ ScrollData& scrollData)
    : m_pStateDelta(NULL)
    , m_pStateUnusedDelta(NULL)
    , m_pStateCurrentOffset(NULL)
    , m_pStateRequestedOffset(NULL)
{
    m_Orientation = orientation;
    m_nRealizedItemsCount = realizedChildrenCount;
    m_nVisualItemsCount = visualChildrenCount;
    m_ComputedOffset = scrollData.m_ComputedOffset;
    m_ArrangedOffset = scrollData.m_ArrangedOffset;
    m_MaxDesiredSize = scrollData.m_MaxDesiredSize;
}

OffsetMemento::~OffsetMemento()
{
    delete m_pStateDelta;
    m_pStateDelta = NULL;

    delete m_pStateUnusedDelta;
    m_pStateUnusedDelta = NULL;

    delete m_pStateCurrentOffset;
    m_pStateCurrentOffset = NULL;

    delete m_pStateRequestedOffset;
    m_pStateRequestedOffset = NULL;
}

DOUBLE OffsetMemento::get_Delta()
{
    if (m_pStateDelta)
    {
        return *m_pStateDelta;
    }
    return 0.0;
}
_Check_return_ 
HRESULT 
OffsetMemento::put_Delta(_In_ DOUBLE delta)
{
    HRESULT hr = S_OK;
    IFCEXPECT(!m_pStateDelta);

    m_pStateDelta = new DOUBLE(delta);

Cleanup:
    RRETURN(hr);
}

DOUBLE OffsetMemento::get_UnusedDelta()
{
    if (m_pStateUnusedDelta)
    {
        return *m_pStateUnusedDelta;
    }
    return 0.0;
}
_Check_return_ 
HRESULT 
OffsetMemento::put_UnusedDelta(_In_ DOUBLE unusedDelta)
{
    HRESULT hr = S_OK;
    IFCEXPECT(!m_pStateUnusedDelta);

    m_pStateUnusedDelta = new DOUBLE(unusedDelta);

Cleanup:
    RRETURN(hr);
}


DOUBLE OffsetMemento::get_CurrentOffset()
{
    if (m_pStateCurrentOffset)
    {
        return *m_pStateCurrentOffset;
    }
    return 0.0;
}
_Check_return_ 
HRESULT 
OffsetMemento::put_CurrentOffset(_In_ DOUBLE currentOffset)
{
    HRESULT hr = S_OK;
    IFCEXPECT(!m_pStateCurrentOffset);

    m_pStateCurrentOffset = new DOUBLE(currentOffset);

Cleanup:
    RRETURN(hr);
}

DOUBLE OffsetMemento::get_RequestedOffset()
{
    if (m_pStateRequestedOffset)
    {
        return *m_pStateRequestedOffset;
    }
    return 0.0;
}

_Check_return_ 
HRESULT 
OffsetMemento::put_RequestedOffset(_In_ DOUBLE requestedOffset)
{
    HRESULT hr = S_OK;
    IFCEXPECT(!m_pStateRequestedOffset);

    m_pStateRequestedOffset = new DOUBLE(requestedOffset);

Cleanup:
    RRETURN(hr);
}

BOOLEAN 
OffsetMemento::Equals(_In_ OffsetMemento* pOffsetMemento)
{
    if (!pOffsetMemento)
    {
        return FALSE;
    }

    return pOffsetMemento->m_Orientation == m_Orientation &&
        pOffsetMemento->m_nRealizedItemsCount == m_nRealizedItemsCount &&
        pOffsetMemento->m_nVisualItemsCount == m_nVisualItemsCount &&
        pOffsetMemento->m_MaxDesiredSize.Height == m_MaxDesiredSize.Height &&
        pOffsetMemento->m_MaxDesiredSize.Width == m_MaxDesiredSize.Width &&
        (pOffsetMemento->m_Orientation == xaml_controls::Orientation_Horizontal ?
        pOffsetMemento->m_ComputedOffset.X == m_ComputedOffset.X :
        pOffsetMemento->m_ComputedOffset.Y == m_ComputedOffset.Y) &&
        (pOffsetMemento->m_Orientation == xaml_controls::Orientation_Horizontal ?
        pOffsetMemento->m_ArrangedOffset.X == m_ArrangedOffset.X :
        pOffsetMemento->m_ArrangedOffset.Y == m_ArrangedOffset.Y);
}
