// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <DependencyObjectAbstractionHelpers.h>
#include <DOCollection.h>

#include "FocusChildrenIteratorWrapper.h"

using namespace DirectUI;
using namespace FocusProperties;

bool FocusChildrenIteratorWrapper::TryMoveNext(_Outptr_result_maybenull_ CDependencyObject** childNoRef)
{
    *childNoRef = nullptr;
    bool hasCurrent = false;

    if (m_children)
    {
        const unsigned childrenCount = m_children->GetCount();
        ASSERT(m_index <= childrenCount);

        if (m_index < childrenCount)
        {
            hasCurrent = true;
            *childNoRef = m_children->GetItemImpl(m_index);
        }
        
        ++m_index;
    }
    // If m_children is null, we can't simply assume m_customIterator is not going
    // to be null as well. Indeed, it may be null in the case a UI element overrides
    // GetChildrenInTabFocusOrder and returns null. A null in this case is equivalent
    // to an empty list.
    else if (m_customIterator)
    {
        BOOLEAN customIteratorHasCurrent;

        if (SUCCEEDED(m_customIterator->get_HasCurrent(&customIteratorHasCurrent)) &&
            customIteratorHasCurrent)
        {
            hasCurrent = true;
            ctl::ComPtr<xaml::IDependencyObject> current;
            if (SUCCEEDED(m_customIterator->get_Current(&current)))
            {
                *childNoRef = DependencyObjectAbstractionHelpers::GetHandle(DependencyObjectAbstractionHelpers::IDOtoDO(current.Get()));
                VERIFYHR(m_customIterator->MoveNext(&customIteratorHasCurrent));
            }
        }
    }

    return hasCurrent;
}