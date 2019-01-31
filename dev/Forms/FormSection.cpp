// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "FormSection.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "Vector.h"
#include "VectorIterator.h"

FormSection::FormSection()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Forms);
}

winrt::Size FormSection::MeasureOverride(winrt::Size const& availableSize)
{
    // ### temporary -- these would be properties
    const winrt::Thickness itemPadding = winrt::Thickness{ 0, 4, 0, 4 };

    auto children = Children();
    float height = 0.0;

    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto child = children.GetAt(i);
        child.Measure(availableSize);

        // ### this is obviously wrong because it's not taking buddies into account but who cares for now...
        height += child.DesiredSize().Height + (float)itemPadding.Top + (float)itemPadding.Bottom;
    }

    // ### for now
    return winrt::Size(500.0f, height);
}

winrt::Size FormSection::ArrangeOverride(winrt::Size const& finalSize)
{
    //### probably need this sort of thing eventually
    //if (m_isLayoutInProgress)
    //{
    //    throw winrt::hresult_error(E_FAIL, L"Reentrancy detected during layout.");
    //}

    // ### temporary -- these would be properties
    const winrt::Thickness itemPadding = winrt::Thickness{ 0, 4, 0, 4 };

    auto children = Children();
    float y = 0.0;

    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto child = children.GetAt(i);
        float width = child.DesiredSize().Width;
        float height = child.DesiredSize().Height;

        child.Arrange(winrt::Rect(0.0f + (float)itemPadding.Left, y + (float)itemPadding.Top, 500.0f, height)); // ### maaaaagic

        y += height + (float)itemPadding.Top + (float)itemPadding.Bottom;
    }

    // ### for now
    return finalSize;
}

void FormSection::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
