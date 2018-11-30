// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "IFlowLayoutAlgorithmDelegates.h"
#include "FlowLayoutAlgorithm.h"
#include "UniformGridLayoutState.h"

CppWinRTActivatableClassWithBasicFactory(UniformGridLayoutState);

void UniformGridLayoutState::InitializeForContext(
    const winrt::VirtualizingLayoutContext& context,
    IFlowLayoutAlgorithmDelegates* callbacks)
{
    m_flowAlgorithm.InitializeForContext(context, callbacks);
    context.LayoutStateCore(*this);
}

void UniformGridLayoutState::UninitializeForContext(const winrt::VirtualizingLayoutContext& context)
{
    m_flowAlgorithm.UninitializeForContext(context);

    if (m_cachedFirstElement)
    {
        context.RecycleElement(m_cachedFirstElement);
    }
}

void UniformGridLayoutState::EnsureElementSize(const winrt::Size availableSize, const winrt::VirtualizingLayoutContext& context, const double layoutItemWidth, const double LayoutItemHeight)
{
    // If the first element is realized we don't need to cache it or to get it from the context
    if (auto realizedElement = m_flowAlgorithm.GetElementIfRealized(0))
    {
        realizedElement.Measure(availableSize);
        SetSize(realizedElement, layoutItemWidth, LayoutItemHeight);
        m_cachedFirstElement = nullptr;
    }
    else
    {
        if (!m_cachedFirstElement)
        {
            // we only cache if we aren't realizing it
            m_cachedFirstElement = context.GetOrCreateElementAt(0, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle); // expensive
        }

        m_cachedFirstElement.Measure(availableSize);
        SetSize(m_cachedFirstElement, layoutItemWidth, LayoutItemHeight);

        // See if we can move ownership to the flow algorithm. If we can, we do not need a local cache.
        bool added = m_flowAlgorithm.TryAddElement0(m_cachedFirstElement);
        if (added)
        {
            m_cachedFirstElement = nullptr;
        }
    }
}

void UniformGridLayoutState::SetSize(winrt::UIElement UIElement, const double layoutItemWidth, const double LayoutItemHeight)
{
    m_effectiveItemWidth = (std::isnan(layoutItemWidth) ? UIElement.DesiredSize().Width : layoutItemWidth);
    m_effectiveItemHeight = (std::isnan(LayoutItemHeight) ? UIElement.DesiredSize().Height : LayoutItemHeight);
}

void UniformGridLayoutState::EnsureFirstElementOwnership()
{
    if (m_flowAlgorithm.GetElementIfRealized(0))
    {
        m_cachedFirstElement = nullptr;
    }
}

void UniformGridLayoutState::ClearElementOnDataSourceChange(winrt::VirtualizingLayoutContext const& context, winrt::NotifyCollectionChangedEventArgs const& args)
{
    if (m_cachedFirstElement)
    {
        bool shouldClear = false;
        switch (args.Action())
        {
        case winrt::NotifyCollectionChangedAction::Add:
            shouldClear = args.NewStartingIndex() == 0;
            break;

        case winrt::NotifyCollectionChangedAction::Replace:
            shouldClear = args.NewStartingIndex() == 0 || args.OldStartingIndex() == 0;
            break;

        case winrt::NotifyCollectionChangedAction::Remove:
            shouldClear = args.OldStartingIndex() == 0;
            break;

        case winrt::NotifyCollectionChangedAction::Reset:
            shouldClear = true;
            break;

        case winrt::NotifyCollectionChangedAction::Move:
            throw winrt::hresult_not_implemented();
            break;
        }

        if (shouldClear)
        {
            context.RecycleElement(m_cachedFirstElement);
            m_cachedFirstElement = nullptr;
        }
    }
}