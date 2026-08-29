// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "palgfx.h"
#include "UIElement.h"
#include "TransitionTarget.h"
#include "Indexes.g.h"
#include "EnumDefs.g.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include <DCompTreeHost.h>
#include <MUX-ETWEvents.h>

xref_ptr<WUComp::IVisual> HWCompNode::GetWUCVisual() const
{
    // This is only implemented in WinRT comp nodes, which should be the only type that exists.
    ASSERT(false);
    return nullptr;
}

xref_ptr<WUComp::IVisual> HWCompNode::GetWUCVisualInMainTree() const
{
    return GetWUCVisual();
}

bool HWCompNode::HasHitTestVisibleContent() const
{
    if (OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>())
    {
        const HWCompTreeNode* compTreeNode = static_cast<const HWCompTreeNode*>(this);
        return compTreeNode->HasHitTestVisibleContentInSubtree();
    }
    else if (OfTypeByIndex<KnownTypeIndex::HWCompRenderDataNode>())
    {
        const HWCompRenderDataNode* renderDataNode = static_cast<const HWCompRenderDataNode*>(this);
        return renderDataNode->HasHitTestVisibleVisuals();
    }
    else
    {
        // It's something like a WebView. Assume they have content.
        return true;
    }
}

bool HWCompTreeNode::HasHitTestVisibleContentInSubtree() const
{
    for (HWCompNode* child : m_children)
    {
        if (child->HasHitTestVisibleContent())
        {
            return true;
        }
    }
    return false;
}

void HWCompTreeNode::NWPropagateDirtyFlag(DirtyFlags flags)
{
    __super::NWPropagateDirtyFlag(flags);
    m_isDCompVisualsDirty = true;
}

bool HWCompWinRTVisualRenderDataNode::HasHitTestVisibleVisuals() const
{
    return HasHitTestVisibleSpriteVisuals(m_containerVisual.get());
}

/* static */ bool HWCompWinRTVisualRenderDataNode::HasHitTestVisibleSpriteVisuals(_In_ WUComp::IContainerVisual* containerVisual)
{
    wrl::ComPtr<WUComp::IVisualCollection> visualCollection;
    IFCFAILFAST(containerVisual->get_Children(&visualCollection));

    wrl::ComPtr<wfc::IIterable<WUComp::Visual*>> iterable;
    IFCFAILFAST(visualCollection.As(&iterable));

    wrl::ComPtr<wfc::IIterator<WUComp::Visual*>> iterator;
    IFCFAILFAST(iterable->First(&iterator));

    boolean hasCurrent = false;
    IFCFAILFAST(iterator->get_HasCurrent(&hasCurrent));

    while (hasCurrent)
    {
        wrl::ComPtr<WUComp::IVisual> visual;
        IFCFAILFAST(iterator->get_Current(&visual));

        wrl::ComPtr<WUComp::ISpriteVisual> spriteVisual;
        wrl::ComPtr<WUComp::IShapeVisual> shapeVisual;

        wrl::ComPtr<ixp::IVisual3> visual3;
        IFCFAILFAST(visual.As(&visual3));

        boolean isHitTestVisible = true;
        IFCFAILFAST(visual3->get_IsHitTestVisible(&isHitTestVisible));

        if (isHitTestVisible
            // Also check that this is a sprite or a shape visual. With synchronous comp tree updates, SpriteVisuals
            // for UIElements and ContainerVisuals for comp nodes get mixed in the same VisualCollection. We want to
            // ignore comp node visuals here. We'll recursively walk down separately and check them for SpriteVisuals
            // via HWCompTreeNode::HasHitTestVisibleContentInSubtree.
            && (SUCCEEDED(visual.As(&spriteVisual)) || SUCCEEDED(visual.As(&shapeVisual))))
        {
            return true;
        }

        IFCFAILFAST(iterator->MoveNext(&hasCurrent));
    }

    return false;
}
