// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "Scroller.h"
#include "ScrollerTrace.h"
#include "ScrollerAnchorRequestedEventArgs.h"

ScrollerAnchorRequestedEventArgs::ScrollerAnchorRequestedEventArgs(const winrt::Scroller& scroller)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, scroller);

    m_scroller.set(scroller);
}

#pragma region IScrollerAnchorRequestedEventArgs

winrt::IVector<winrt::UIElement> ScrollerAnchorRequestedEventArgs::AnchorCandidates()
{
    return m_anchorCandidates.get();
}

winrt::UIElement ScrollerAnchorRequestedEventArgs::AnchorElement()
{
    return m_anchorElement.get();
}

void ScrollerAnchorRequestedEventArgs::AnchorElement(winrt::UIElement const& value)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);

    const winrt::UIElement& anchorElement{ value };
    com_ptr<Scroller> scroller = winrt::get_self<Scroller>(m_scroller.get())->get_strong();

    if (!anchorElement || scroller->IsElementValidAnchor(anchorElement))
    {
        m_anchorElement.set(anchorElement);
    }
    else
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

#pragma endregion

winrt::IVector<winrt::UIElement> ScrollerAnchorRequestedEventArgs::GetAnchorCandidates()
{
    return m_anchorCandidates.get();
}

void ScrollerAnchorRequestedEventArgs::SetAnchorCandidates(const std::vector<tracker_ref<winrt::UIElement>>& anchorCandidates)
{
    winrt::IVector<winrt::UIElement> anchorCandidatesTmp = winrt::make<Vector<winrt::UIElement, MakeVectorParam<VectorFlag::DependencyObjectBase>()>>();
        
    for (tracker_ref<winrt::UIElement> anchorCandidate : anchorCandidates)
    {
        anchorCandidatesTmp.Append(anchorCandidate.get());
    }
    m_anchorCandidates.set(anchorCandidatesTmp);
}

winrt::UIElement ScrollerAnchorRequestedEventArgs::GetAnchorElement() const
{
    return m_anchorElement.get();
}

void ScrollerAnchorRequestedEventArgs::SetAnchorElement(const winrt::UIElement& anchorElement)
{
    m_anchorElement.set(anchorElement);
}
